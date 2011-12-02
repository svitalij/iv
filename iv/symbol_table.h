#ifndef IV_SYMBOLTABLE_H_
#define IV_SYMBOLTABLE_H_
#include <string>
#include <vector>
#include <iv/detail/unordered_map.h>
#include <iv/ustring.h>
#include <iv/conversions.h>
#include <iv/symbol.h>
namespace iv {
namespace core {

class SymbolTable {
 public:
  typedef std::unordered_set<SymbolStringHolder> Set;

  SymbolTable()
    : set_() {
    // insert default symbols
#define V(sym) InsertDefaults(symbol::sym());
    IV_DEFAULT_SYMBOLS(V)
#undef V
  }

  ~SymbolTable() {
    for (Set::const_iterator it = set_.begin(),
         last = set_.end(); it != last; ++it) {
      const Symbol sym = detail::MakeSymbol(it->symbolized_);
      if (!symbol::DefaultSymbolProvider::Instance()->IsDefaultSymbol(sym)) {
        delete symbol::GetStringFromSymbol(sym);
      }
    }
  }

  template<class CharT>
  inline Symbol Lookup(const CharT* str) {
    using std::char_traits;
    return Lookup(core::BasicStringPiece<CharT>(str));
  }

  template<class String>
  inline Symbol Lookup(const String& str) {
    uint32_t index;
    if (core::ConvertToUInt32(str.begin(), str.end(), &index)) {
      return symbol::MakeSymbolFromIndex(index);
    }
    const core::UString target(str.begin(), str.end());
    SymbolStringHolder holder = { &target };
    typename Set::const_iterator it = set_.find(holder);
    if (it != set_.end()) {
      return detail::MakeSymbol(it->symbolized_);
    } else {
      holder.symbolized_ = new core::UString(target);
      set_.insert(holder);
      return detail::MakeSymbol(holder.symbolized_);
    }
  }

 private:
  void InsertDefaults(Symbol sym) {
    if (symbol::IsStringSymbol(sym)) {
      SymbolStringHolder holder;
      holder.symbolized_ = symbol::GetStringFromSymbol(sym);
      assert(set_.find(holder) == set_.end());
      set_.insert(holder);
    }
  }

  Set set_;
};

} }  // namespace iv::core
#endif  // IV_SYMBOLTABLE_H_