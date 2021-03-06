#ifndef IV_AERO_VM_H_
#define IV_AERO_VM_H_
#include <vector>
#include <algorithm>
#include <memory>
#include <iv/platform.h>
#include <iv/noncopyable.h>
#include <iv/string_view.h>
#include <iv/os_allocator.h>
#include <iv/aero/code.h>
#include <iv/aero/character.h>
#include <iv/aero/utility.h>
namespace iv {
namespace aero {

template<typename Piece>
inline bool IsWordSeparatorPrev(const Piece& subject,
                                std::size_t current_position) {
  if (current_position == 0 || current_position > subject.size()) {
    return false;
  }
  return character::IsWord(subject[current_position - 1]);
}

template<typename Piece>
inline bool IsWordSeparator(const Piece& subject,
                            std::size_t current_position) {
  if (current_position >= subject.size()) {
    return false;
  }
  return character::IsWord(subject[current_position]);
}

class VM : private core::Noncopyable<VM> {
 public:
  static const uint32_t kInitialStackSize = 8096;
  static const uint32_t kInitialStateSize = 64;

  VM()
    : stack_base_pointer_for_jit_(nullptr)
    , state_pointer_for_jit_(
          reinterpret_cast<int*>(std::malloc(kInitialStateSize * sizeof(int))))
    , stack_size_(kInitialStackSize)
    , state_size_(kInitialStateSize)
    , stack_(kInitialStackSize)
#if !defined(IV_ENABLE_JIT)
    , state_()
#endif
  {
    stack_base_pointer_for_jit_ = stack_.data();
  }

  ~VM() {
    std::free(state_pointer_for_jit_);
  }

  // JIT interface for VM
  static int* NewStateForJIT(VM* vm, int offset) {
    return vm->NewState(offset);
  }

#if defined(IV_ENABLE_JIT)
  int Execute(Code* code, const core::u16string_view& subject,
              int* captures, int offset) {
    // Currently disabled.
    // if (code->simple()) {
    //   return code->simple()->Execute(subject, captures, offset);
    // }
    return code->jit()->Execute(this, code, subject, captures, offset);
  }

  int Execute(Code* code, const core::string_view& subject,
              int* captures, int offset) {
    // Currently disabled.
    // if (code->simple()) {
    //   return code->simple()->Execute(subject, captures, offset);
    // }
    return code->jit()->Execute(this, code, subject, captures, offset);
  }
#else
  int Execute(Code* code, const core::u16string_view& subject,
              int* captures, int offset) {
    // Currently disabled.
    // if (code->simple()) {
    //   return code->simple()->Execute(subject, captures, offset);
    // }
    return ExecuteImpl(code, subject, captures, offset);
  }

  int Execute(Code* code, const core::string_view& subject,
              int* captures, int offset) {
    // Currently disabled.
    // if (code->simple()) {
    //   return code->simple()->Execute(subject, captures, offset);
    // }
    return ExecuteImpl(code, subject, captures, offset);
  }

 private:
  template<typename Piece>
  int ExecuteImpl(Code* code, const Piece& subject, int* captures, int offset) {
    const int size = subject.size();
    const uint16_t filter = code->filter();
    if (!filter) {
      // normal path
      do {
        const int res = Main(code, subject, captures, offset);
        if (res == AERO_SUCCESS || res == AERO_ERROR) {
          return res;
        } else {
          ++offset;
        }
      } while (offset <= size);
    } else if (code->IsQuickCheckOneChar()) {
      // one char check path
      while (offset < size) {
        const char16_t ch = subject[offset];
        if (ch != filter) {
          ++offset;
        } else {
          const int res = Main(code, subject, captures, offset);
          if (res == AERO_SUCCESS || res == AERO_ERROR) {
            return res;
          } else {
            ++offset;
          }
        }
      }
    } else {
      // bloom filter path
      while (offset < size) {
        const char16_t ch = subject[offset];
        if ((filter & ch) != ch) {
          ++offset;
        } else {
          const int res = Main(code, subject, captures, offset);
          if (res == AERO_SUCCESS || res == AERO_ERROR) {
            return res;
          } else {
            ++offset;
          }
        }
      }
    }
    return AERO_FAILURE;
  }
#endif

  uint64_t state_size() const { return state_size_; }

 private:
  int* NewState(std::size_t offset) {
    do {
      if (offset <= stack_.size()) {
        return stack_base_pointer_for_jit_ + offset;
      }
      // overflow
      stack_.resize(stack_.size() * 2, -1);
      stack_base_pointer_for_jit_ = stack_.data();
      stack_size_ = stack_.size();
    } while (true);
    return nullptr;
  }

  int* stack_base_pointer_for_jit_;
  int* state_pointer_for_jit_;
  uint64_t stack_size_;
  uint64_t state_size_;
  std::vector<int> stack_;
#if !defined(IV_ENABLE_JIT)
  std::vector<int> state_;

  template<typename Piece>
  int Main(Code* code, const Piece& subject,
           int* captures, std::size_t current_position);
#endif
};

#if !defined(IV_ENABLE_JIT)
// #define DEFINE_OPCODE(op) case OP::op: printf("%s\n", #op);

#define DEFINE_OPCODE(op)\
  case OP::op:
#define DISPATCH() goto aero_main_loop;
#define DISPATCH_NEXT()\
  do {\
    const uint32_t next = OP::GetLength(instr);\
    instr += next;\
  } while (0);\
  DISPATCH()
#define BACKTRACK() goto aero_backtrack;

template<typename Piece>
inline int VM::Main(Code* code, const Piece& subject,
                    int* captures, std::size_t current_position) {
  assert(code->captures() >= 1);
  // captures and counters and jump target
  const std::size_t size = code->captures() * 2 + code->counters() + 1;
  // state layout is following
  // [ captures ][ captures ][ target ]
  state_.assign(size, kUndefined);
  state_[0] = current_position;
  int* sp = stack_.data();
  const uint8_t* instr = code->data();
  const uint8_t* const first_instr = instr;

  for (;;) {
    // fetch opcode
    aero_main_loop:
    switch (instr[0]) {
      DEFINE_OPCODE(PUSH_BACKTRACK) {
        if ((sp = NewState((sp - stack_.data()) + size))) {
          // copy state and push to backtrack stack
          int* target = sp - size;
          std::copy(state_.begin(), state_.begin() + size - 1, target);
          target[size - 1] = static_cast<int>(Load4Bytes(instr + 1));
          target[1] = current_position;
        } else {
          // stack overflowed
          return AERO_ERROR;
        }
        DISPATCH_NEXT();
      }

      DEFINE_OPCODE(START_CAPTURE) {
        const uint32_t target = Load4Bytes(instr + 1);
        state_[target * 2] = current_position;
        state_[target * 2 + 1] = kUndefined;
        DISPATCH_NEXT();
      }

      DEFINE_OPCODE(END_CAPTURE) {
        const uint32_t target = Load4Bytes(instr + 1);
        state_[target * 2 + 1] = current_position;
        DISPATCH_NEXT();
      }

      DEFINE_OPCODE(CLEAR_CAPTURES) {
        const uint32_t from = Load4Bytes(instr + 1);
        const uint32_t to = Load4Bytes(instr + 5);
        std::fill_n(state_.begin() + from * 2, (to - from) * 2, kUndefined);
        DISPATCH_NEXT();
      }

      DEFINE_OPCODE(BACK_REFERENCE) {
        const uint16_t ref = Load2Bytes(instr + 1);
        assert(ref != 0);  // limited by parser
        if (ref < code->captures() && state_[ref * 2 + 1] != kUndefined) {
          assert(state_[ref * 2] != kUndefined);
          assert(state_[ref * 2 + 1] != kUndefined);
          const int start = state_[ref * 2];
          const int length = state_[ref * 2 + 1] - start;
          if (current_position + length > subject.size()) {
            // out of range
            BACKTRACK();
          }
          if (std::equal(
                  subject.begin() + start,
                  subject.begin() + start + length,
                  subject.begin() + current_position)) {
            current_position += length;
          } else {
            BACKTRACK();
          }
        }
        DISPATCH_NEXT();
      }

      DEFINE_OPCODE(BACK_REFERENCE_IGNORE_CASE) {
        const uint16_t ref = Load2Bytes(instr + 1);
        if (ref < code->captures() &&
            state_[ref * 2] != kUndefined &&
            state_[ref * 2 + 1] != kUndefined) {
          const int start = state_[ref * 2];
          const int length = state_[ref * 2 + 1] - start;
          if (current_position + length > subject.size()) {
            // out of range
            BACKTRACK();
          }
          bool matched = true;
          for (typename Piece::const_iterator it = subject.begin() + start,
               last = subject.begin() + start + length; it != last;
               ++it, ++current_position) {
            const char16_t current = subject[current_position];
            if (current == *it ||
                current == core::character::ToUpperCase(*it) ||
                current == core::character::ToLowerCase(*it)) {
              continue;
            }
            BACKTRACK();
          }
        }
        DISPATCH_NEXT();
      }

      DEFINE_OPCODE(STORE_POSITION) {
        state_[code->captures() * 2 + Load4Bytes(instr + 1)] =
            static_cast<int>(current_position);
        DISPATCH_NEXT();
      }

      DEFINE_OPCODE(POSITION_TEST) {
        if (current_position !=
            static_cast<std::size_t>(
                state_[code->captures() * 2 + Load4Bytes(instr + 1)])) {
          DISPATCH_NEXT();
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(COUNTER_ZERO) {
        state_[code->captures() * 2 + Load4Bytes(instr + 1)] = 0;
        DISPATCH_NEXT();
      }

      DEFINE_OPCODE(COUNTER_NEXT) {
        // COUNTER_NEXT | COUNTER_TARGET | MAX | JUMP_TARGET
        const int32_t max = static_cast<int32_t>(Load4Bytes(instr + 5));
        if (++state_[code->captures() * 2 + Load4Bytes(instr + 1)] < max) {
          instr = first_instr + Load4Bytes(instr + 9);
          DISPATCH();
        }
        DISPATCH_NEXT();
      }

      DEFINE_OPCODE(CHECK_1BYTE_CHAR) {
        if (current_position < subject.size() &&
            subject[current_position] == Load1Bytes(instr + 1)) {
          ++current_position;
          DISPATCH_NEXT();
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(CHECK_2BYTE_CHAR) {
        if (current_position < subject.size() &&
            subject[current_position] == Load2Bytes(instr + 1)) {
          ++current_position;
          DISPATCH_NEXT();
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(CHECK_2CHAR_OR) {
        if (current_position < subject.size()) {
          const char16_t ch = subject[current_position];
          if (ch == Load2Bytes(instr + 1) || ch == Load2Bytes(instr + 3)) {
            ++current_position;
            DISPATCH_NEXT();
          }
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(CHECK_3CHAR_OR) {
        if (current_position < subject.size()) {
          const char16_t ch = subject[current_position];
          if (ch == Load2Bytes(instr + 1) ||
              ch == Load2Bytes(instr + 3) ||
              ch == Load2Bytes(instr + 5)) {
            ++current_position;
            DISPATCH_NEXT();
          }
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(CHECK_4CHAR_OR) {
        if (current_position < subject.size()) {
          const char16_t ch = subject[current_position];
          if (ch == Load2Bytes(instr + 1) ||
              ch == Load2Bytes(instr + 3) ||
              ch == Load2Bytes(instr + 5) ||
              ch == Load2Bytes(instr + 7)) {
            ++current_position;
            DISPATCH_NEXT();
          }
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(STORE_SP) {
        state_[code->captures() * 2 + Load4Bytes(instr + 1)]
            = sp - stack_.data();
        DISPATCH_NEXT();
      }

      DEFINE_OPCODE(ASSERTION_SUCCESS) {
        int* previous = sp =
            stack_.data() +
            state_[code->captures() * 2 + Load4Bytes(instr + 1)];
        current_position = previous[1];
        instr = first_instr + Load4Bytes(instr + 5);
        DISPATCH();
      }

      DEFINE_OPCODE(ASSERTION_FAILURE) {
        sp =
            stack_.data() +
            state_[code->captures() * 2 + Load4Bytes(instr + 1)];
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_BOB) {
        if (current_position == 0) {
          DISPATCH_NEXT();
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_EOB) {
        if (current_position == subject.size()) {
          DISPATCH_NEXT();
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_BOL) {
        if (current_position == 0 ||
            core::character::IsLineTerminator(subject[current_position - 1])) {
          DISPATCH_NEXT();
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_EOL) {
        if (current_position == subject.size() ||
            core::character::IsLineTerminator(subject[current_position])) {
          DISPATCH_NEXT();
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_WORD_BOUNDARY) {
        if (IsWordSeparatorPrev(subject, current_position) !=
            IsWordSeparator(subject, current_position)) {
          DISPATCH_NEXT();
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(ASSERTION_WORD_BOUNDARY_INVERTED) {
        if (IsWordSeparatorPrev(subject, current_position) ==
            IsWordSeparator(subject, current_position)) {
          DISPATCH_NEXT();
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(CHECK_RANGE) {
        if (current_position < subject.size()) {
          const char16_t ch = subject[current_position];
          const uint32_t length = Load4Bytes(instr + 1);
          const uint32_t counts = Load4Bytes(instr + 5);
          for (std::size_t i = 0; i < length; i += 4) {
            const char16_t start = Load2Bytes(instr + 5 + 4 + i);
            if (ch < start) {
              break;
            }
            const char16_t finish = Load2Bytes(instr + 5 + 4 + i + 2);
            if (ch <= finish) {
              ++current_position;
              DISPATCH_NEXT();
            }
          }
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(CHECK_RANGE_INVERTED) {
        if (current_position < subject.size()) {
          const char16_t ch = subject[current_position];
          const uint32_t length = Load4Bytes(instr + 1);
          const uint32_t counts = Load4Bytes(instr + 5);
          for (std::size_t i = 0; i < length; i += 4) {
            const char16_t start = Load2Bytes(instr + 5 + 4 + i);
            if (ch < start) {
              break;
            }
            const char16_t finish = Load2Bytes(instr + 5 + 4 + i + 2);
            if (ch <= finish) {
              BACKTRACK();
            }
          }
          ++current_position;
          DISPATCH_NEXT();
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(CHECK_N_CHARS) {
        const uint32_t length = Load4Bytes(instr + 1);
        assert(length > 0);
        if (current_position + (length - 1) < subject.size()) {
          for (uint32_t i = 0; i < length; ++i) {
            const auto current = subject[current_position + i];
            const char16_t target = Load2Bytes(instr + 5 + i * 2);
            if (current == target) {
              continue;
            }

            if (code->IsIgnoreCase()) {
              const char16_t uu = core::character::ToUpperCase(target);
              const char16_t lu = core::character::ToLowerCase(target);
              if (current == uu || current == lu) {
                continue;
              }
            }
            BACKTRACK();
          }
          current_position += length;
          DISPATCH_NEXT();
        }
        BACKTRACK();
      }

      DEFINE_OPCODE(FAILURE) {
        BACKTRACK();
      }

      DEFINE_OPCODE(SUCCESS) {
        state_[1] = current_position;
        std::copy(state_.begin(),
                  state_.begin() + code->captures() * 2, captures);
        return AERO_SUCCESS;
      }

      DEFINE_OPCODE(JUMP) {
        instr = first_instr + Load4Bytes(instr + 1);
        DISPATCH();
      }
    }

    // backtrack stack unwind
    aero_backtrack:
    if (sp > stack_.data()) {
      sp -= size;
      std::copy(sp, sp + size, state_.begin());
      current_position = state_[1];
      instr = first_instr + state_[size - 1];
      DISPATCH();
    }
    return AERO_FAILURE;
  }
  return AERO_SUCCESS;  // makes compiler happy
}
#undef DEFINE_OPCODE
#undef DISPATCH
#undef DISPATCH_NEXT
#undef BACKTRACK
#endif

} }  // namespace iv::aero
#endif  // IV_AERO_VM_H_
