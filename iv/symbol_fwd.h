#ifndef IV_SYMBOL_FWD_H_
#define IV_SYMBOL_FWD_H_
#include <functional>
#include <iv/detail/cstdint.h>
#include <iv/detail/cinttypes.h>
#include <iv/detail/functional.h>
#include <iv/ustring.h>
#include <iv/ustringpiece.h>
#include <iv/stringpiece.h>
#include <iv/byteorder.h>
#include <iv/platform.h>
#include <iv/static_assert.h>
namespace iv {
namespace core {
namespace detail {

template<std::size_t PointerSize, bool IsLittle>
struct SymbolLayout;

static const uint32_t kSymbolIsIndex = 0xFFFF;

template<>
struct SymbolLayout<4, true> {
  typedef SymbolLayout<4, true> this_type;
  union {
    struct {
      uint32_t high_;
      uint32_t low_;
    } index_;
    struct {
      const core::UString* str_;
      uint32_t low_;
    } str_;
    uint64_t bytes_;
  };
};

template<>
struct SymbolLayout<8, true> {
  typedef SymbolLayout<8, true> this_type;
  union {
    struct {
      uint32_t high_;
      uint32_t low_;
    } index_;
    struct {
      const core::UString* str_;
    } str_;
    uint64_t bytes_;
  };
};

template<>
struct SymbolLayout<4, false> {
  typedef SymbolLayout<4, false> this_type;
  union {
    struct {
      uint32_t low_;
      uint32_t high_;
    } index_;
    struct {
      uint32_t low_;
      const core::UString* str_;
    } str_;
    uint64_t bytes_;
  };
};

template<>
struct SymbolLayout<8, false> {
  typedef SymbolLayout<8, false> this_type;
  union {
    struct {
      uint32_t low_;
      uint32_t high_;
    } index_;
    struct {
      const core::UString* str_;
    } str_;
    uint64_t bytes_;
  };
};

typedef detail::SymbolLayout<
    core::Size::kPointerSize,
    core::kLittleEndian> Symbol;

inline Symbol MakeSymbol(const core::UString* str) {
  Symbol symbol = { };
  symbol.str_.str_ = str;
  return symbol;
}

inline Symbol MakeSymbol(uint32_t index) {
  Symbol symbol;
  symbol.index_.high_ = index;
  symbol.index_.low_ = kSymbolIsIndex;
  return symbol;
}

inline bool operator==(Symbol x, Symbol y) {
  return x.bytes_ == y.bytes_;
}

inline bool operator!=(Symbol x, Symbol y) {
  return x.bytes_ != y.bytes_;
}

inline bool operator<(Symbol x, Symbol y) {
  return x.bytes_ < y.bytes_;
}

inline bool operator>(Symbol x, Symbol y) {
  return x.bytes_ > y.bytes_;
}

inline bool operator<=(Symbol x, Symbol y) {
  return x.bytes_ <= y.bytes_;
}

inline bool operator>=(Symbol x, Symbol y) {
  return x.bytes_ >= y.bytes_;
}

}  // namespace detail

typedef detail::Symbol Symbol;

#if defined(IV_COMPILER_GCC) && (IV_COMPILER_GCC >= 40300)
IV_STATIC_ASSERT(std::is_pod<Symbol>::value);
#endif

struct SymbolStringHolder {
  const core::UString* symbolized_;
};

inline bool operator==(SymbolStringHolder x, SymbolStringHolder y) {
  return (*x.symbolized_) == (*y.symbolized_);
}

inline bool operator!=(SymbolStringHolder x, SymbolStringHolder y) {
  return (*x.symbolized_) != (*y.symbolized_);
}

inline bool operator<(SymbolStringHolder x, SymbolStringHolder y) {
  return (*x.symbolized_) < (*y.symbolized_);
}

inline bool operator>(SymbolStringHolder x, SymbolStringHolder y) {
  return (*x.symbolized_) > (*y.symbolized_);
}

inline bool operator<=(SymbolStringHolder x, SymbolStringHolder y) {
  return (*x.symbolized_) <= (*y.symbolized_);
}

inline bool operator>=(SymbolStringHolder x, SymbolStringHolder y) {
  return (*x.symbolized_) >= (*y.symbolized_);
}

namespace symbol {

inline bool IsIndexSymbol(Symbol sym) {
  return sym.index_.low_ == detail::kSymbolIsIndex;
}

inline bool IsArrayIndexSymbol(Symbol sym) {
  return
      (sym.index_.low_ == detail::kSymbolIsIndex) &&
      (sym.index_.high_ < UINT32_MAX);
}

inline bool IsStringSymbol(Symbol sym) {
  return !IsIndexSymbol(sym);
}

inline Symbol MakeSymbolFromIndex(uint32_t index) {
  return detail::MakeSymbol(index);
}

inline const core::UString* GetStringFromSymbol(Symbol sym) {
  assert(IsStringSymbol(sym));
  return sym.str_.str_;
}

inline uint32_t GetIndexFromSymbol(Symbol sym) {
  assert(IsIndexSymbol(sym));
  return sym.index_.high_;
}

inline core::UString GetIndexStringFromSymbol(Symbol sym) {
  assert(IsIndexSymbol(sym));
  const uint32_t index = GetIndexFromSymbol(sym);
  std::array<char, 15> buffer;
  char* end = core::UInt32ToString(index, buffer.data());
  return core::UString(buffer.data(), end);
}

inline core::UString GetSymbolString(Symbol sym) {
  if (IsIndexSymbol(sym)) {
    return GetIndexStringFromSymbol(sym);
  } else {
    return *GetStringFromSymbol(sym);
  }
}

} } }  // namespace iv::core::symbol
namespace IV_HASH_NAMESPACE_START {

// template specialization for Symbol in std::unordered_map
template<>
struct hash<iv::core::Symbol>
  : public std::unary_function<iv::core::Symbol, std::size_t> {
  inline result_type operator()(const argument_type& x) const {
    return hash<uint64_t>()(x.bytes_);
  }
};

// template specialization for SymbolStringHolder in std::unordered_map
template<>
struct hash<iv::core::SymbolStringHolder>
  : public std::unary_function<iv::core::SymbolStringHolder, std::size_t> {
  inline result_type operator()(const argument_type& x) const {
    return hash<iv::core::UString>()(*x.symbolized_);
  }
};

} IV_HASH_NAMESPACE_END  // namespace std
#endif  // IV_SYMBOL_FWD_H_