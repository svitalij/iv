#ifndef IV_LV5_RAILGUN_CORE_DATA_H_
#define IV_LV5_RAILGUN_CORE_DATA_H_
#include <iv/lv5/railgun/vm_fwd.h>
#include <iv/lv5/railgun/core_data_fwd.h>
#include <iv/lv5/railgun/instruction.h>
namespace iv {
namespace lv5 {
namespace railgun {

inline GC_ms_entry* CoreData::MarkChildren(GC_word* top,
                                           GC_ms_entry* entry,
                                           GC_ms_entry* mark_sp_limit,
                                           GC_word env) {
  if (data_) {
    entry = GC_MARK_AND_PUSH(
        data_,
        entry, mark_sp_limit, reinterpret_cast<void**>(this));
    if (compiled_) {
      for (std::size_t n = 0, len = data_->size(); n < len;) {
        const Instruction& instr = (*data_)[n];
        // loop and search Map pointer operations
        // check global opcodes
        if (VM::IsOP<OP::LOAD_GLOBAL>(instr) ||
            VM::IsOP<OP::STORE_GLOBAL>(instr) ||
            VM::IsOP<OP::DELETE_GLOBAL>(instr) ||
            VM::IsOP<OP::INCREMENT_GLOBAL>(instr) ||
            VM::IsOP<OP::DECREMENT_GLOBAL>(instr) ||
            VM::IsOP<OP::POSTFIX_INCREMENT_GLOBAL>(instr) ||
            VM::IsOP<OP::POSTFIX_DECREMENT_GLOBAL>(instr)) {
          // opcode | dst | index | map | offset
          entry = GC_MARK_AND_PUSH(
              (*data_)[n + 3].map,
              entry, mark_sp_limit, reinterpret_cast<void**>(this));
        } else if (VM::IsOP<OP::LOAD_OBJECT>(instr)) {
          // opcode | index | map
          entry = GC_MARK_AND_PUSH(
              (*data_)[n + 2].map,
              entry, mark_sp_limit, reinterpret_cast<void**>(this));
        } else if (VM::IsOP<OP::STORE_PROP>(instr)) {
          // opcode | base | index | src | nop | nop | nop | nop
          entry = GC_MARK_AND_PUSH(
              (*data_)[n + 4].map,
              entry, mark_sp_limit, reinterpret_cast<void**>(this));
        } else if (VM::IsOP<OP::LOAD_PROP_OWN>(instr)) {
          // opcode | dst | base | name | map | offset | nop | nop
          entry = GC_MARK_AND_PUSH(
              (*data_)[n + 4].map,
              entry, mark_sp_limit, reinterpret_cast<void**>(this));
        } else if (VM::IsOP<OP::LOAD_PROP_PROTO>(instr)) {
          // opcode | dst | base | name | map | map | offset | nop
          entry = GC_MARK_AND_PUSH(
              (*data_)[n + 4].map,
              entry, mark_sp_limit, reinterpret_cast<void**>(this));
          entry = GC_MARK_AND_PUSH(
              (*data_)[n + 5].map,
              entry, mark_sp_limit, reinterpret_cast<void**>(this));
        } else if (VM::IsOP<OP::LOAD_PROP_CHAIN>(instr)) {
          // opcode | dst | base | name | chain | map | offset | nop
          entry = GC_MARK_AND_PUSH(
              (*data_)[n + 4].chain,
              entry, mark_sp_limit, reinterpret_cast<void**>(this));
          entry = GC_MARK_AND_PUSH(
              (*data_)[n + 5].map,
              entry, mark_sp_limit, reinterpret_cast<void**>(this));
        }
        n += instr.GetLength();
      }
    }
  }
  return entry;
}

inline void CoreData::MarkChildren(radio::Core* core) {
  if (compiled_) {
    for (std::size_t n = 0, len = data_->size(); n < len;) {
      const Instruction& instr = (*data_)[n];
      // loop and search Map pointer operations
      // check global opcodes
      if (VM::IsOP<OP::LOAD_GLOBAL>(instr) ||
          VM::IsOP<OP::STORE_GLOBAL>(instr) ||
          VM::IsOP<OP::DELETE_GLOBAL>(instr) ||
          VM::IsOP<OP::INCREMENT_GLOBAL>(instr) ||
          VM::IsOP<OP::DECREMENT_GLOBAL>(instr) ||
          VM::IsOP<OP::POSTFIX_INCREMENT_GLOBAL>(instr) ||
          VM::IsOP<OP::POSTFIX_DECREMENT_GLOBAL>(instr)) {
        // opcode | dst | index | map | offset
        core->MarkCell((*data_)[n + 3].map);
      } else if (VM::IsOP<OP::LOAD_OBJECT>(instr)) {
        // opcode | index | map
        core->MarkCell((*data_)[n + 2].map);
      } else if (VM::IsOP<OP::STORE_PROP>(instr)) {
        // opcode | base | index | src | nop | nop | nop | nop
        core->MarkCell((*data_)[n + 4].map);
      } else if (VM::IsOP<OP::LOAD_PROP_OWN>(instr)) {
        // opcode | dst | base | name | map | offset | nop | nop
        core->MarkCell((*data_)[n + 4].map);
      } else if (VM::IsOP<OP::LOAD_PROP_PROTO>(instr)) {
        // opcode | dst | base | name | map | map | offset | nop
        core->MarkCell((*data_)[n + 4].map);
        core->MarkCell((*data_)[n + 5].map);
      } else if (VM::IsOP<OP::LOAD_PROP_CHAIN>(instr)) {
        // opcode | dst | base | name | chain | map | offset | nop
        // core->MarkCell((*data_)[n + 4].chain);
        core->MarkCell((*data_)[n + 5].map);
      }
      n += instr.GetLength();
    }
  }
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_CORE_DATA_H_
