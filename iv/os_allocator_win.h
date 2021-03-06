#ifndef IV_OS_ALLOCATOR_WIN_H_
#define IV_OS_ALLOCATOR_WIN_H_
#include <windows.h>
namespace iv {
namespace core {

inline void* OSAllocator::Allocate(std::size_t bytes) {
  void* mem = VirtualAlloc(0, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!mem) {
    std::abort();
  }
  return mem;
}

inline void OSAllocator::Deallocate(void* address, std::size_t bytes) {
  const bool res = VirtualFree(address, 0, MEM_RELEASE) != 0;
  if (!res) {
    std::abort();
  }
}

inline void OSAllocator::Commit(void* address, std::size_t bytes) {
  const void* mem = VirtualAlloc(address, bytes, MEM_COMMIT, PAGE_READWRITE);
  if (!mem) {
    std::abort();
  }
}

inline void OSAllocator::Decommit(void* address, std::size_t bytes) {
  const bool res = VirtualFree(address, bytes, MEM_DECOMMIT) != 0;
  if (!res) {
    std::abort();
  }
}

} }  // namespace iv::core
#endif  // IV_OS_ALLOCATOR_WIN_H_
