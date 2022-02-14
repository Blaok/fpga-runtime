#ifndef FPGA_RUNTIME_STREAM_INTERFACE_H_
#define FPGA_RUNTIME_STREAM_INTERFACE_H_

#include <cstddef>

namespace fpga {
namespace internal {

class StreamInterface {
 public:
  virtual ~StreamInterface() = default;
  virtual void Read(void* ptr, size_t size, bool eot) = 0;
  virtual void Write(const void* ptr, size_t size, bool eot) = 0;
};

}  // namespace internal
}  // namespace fpga

#endif  // FPGA_RUNTIME_STREAM_INTERFACE_H_
