#ifndef FPGA_RUNTIME_BUFFER_H_
#define FPGA_RUNTIME_BUFFER_H_

#include <cstddef>

#include "frt/tag.h"

namespace fpga {
namespace internal {

template <typename T, Tag tag>
class Buffer {
 public:
  Buffer(T* ptr, size_t n) : ptr_(ptr), n_(n) {}
  T* Get() const { return ptr_; }
  size_t SizeInBytes() const { return n_ * sizeof(T); }

 private:
  T* const ptr_;
  const size_t n_;
};

}  // namespace internal
}  // namespace fpga

#endif  // FPGA_RUNTIME_BUFFER_H_
