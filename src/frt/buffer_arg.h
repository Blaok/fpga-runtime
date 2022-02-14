#ifndef FPGA_RUNTIME_BUFFER_ARG_H_
#define FPGA_RUNTIME_BUFFER_ARG_H_

#include <cstddef>

#include "frt/buffer.h"
#include "frt/tag.h"

namespace fpga {
namespace internal {

class BufferArg {
 public:
  template <typename T, Tag tag>
  BufferArg(Buffer<T, tag> buffer)
      : ptr_(reinterpret_cast<char*>(buffer.Get())),
        size_(sizeof(T)),
        n_(buffer.SizeInBytes() / sizeof(T)) {}

  BufferArg() = default;
  BufferArg(const BufferArg&) = default;
  BufferArg& operator=(const BufferArg&) = default;
  BufferArg(BufferArg&&) = default;
  BufferArg& operator=(BufferArg&&) = default;

  char* Get() const { return ptr_; }
  size_t SizeInCount() const { return n_; }
  size_t SizeInBytes() const { return size_ * n_; }

 private:
  char* ptr_;
  size_t size_;
  size_t n_;
};

}  // namespace internal
}  // namespace fpga

#endif  // FPGA_RUNTIME_BUFFER_ARG_H_
