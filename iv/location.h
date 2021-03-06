#ifndef IV_LOCATION_H_
#define IV_LOCATION_H_
#include <cstddef>
#include <iv/detail/type_traits.h>
#include <iv/platform.h>
namespace iv {
namespace core {

struct Location {
  std::size_t begin_position_;
  std::size_t end_position_;
  inline std::size_t begin_position() const {
    return begin_position_;
  }
  inline std::size_t end_position() const {
    return end_position_;
  }
  inline void set_begin_position(std::size_t begin) {
    begin_position_ = begin;
  }
  inline void set_end_position(std::size_t end) {
    end_position_ = end;
  }
};

#if defined(IV_COMPILER_GCC) && (IV_COMPILER_GCC >= 40300)
static_assert(std::is_pod<Location>::value, "Location should be POD");
#endif

} }  // namespace iv::core
#endif  // IV_LOCATION_H_
