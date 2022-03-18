#ifndef FPGA_RUNTIME_XILINX_OPENCL_STREAM_H_
#define FPGA_RUNTIME_XILINX_OPENCL_STREAM_H_

#include <string>

#include <CL/cl2.hpp>

#include "frt/stream_interface.h"
#include "frt/tag.h"

extern "C" {
struct _cl_stream;
}  // extern "C"

namespace fpga {
namespace internal {

class XilinxOpenclStream : public StreamInterface {
 public:
  XilinxOpenclStream(const std::string& name, cl::Device device,
                     cl::Kernel kernel, int index, Tag tag);
  XilinxOpenclStream(const XilinxOpenclStream&) = delete;
  XilinxOpenclStream& operator=(const XilinxOpenclStream&) = delete;
  XilinxOpenclStream(XilinxOpenclStream&&) = delete;
  XilinxOpenclStream& operator=(XilinxOpenclStream&&) = delete;
  ~XilinxOpenclStream() override;

  void Read(void* ptr, size_t size, bool eot) override;
  void Write(const void* ptr, size_t size, bool eot) override;

 private:
  const std::string& name_;
  _cl_stream* stream_ = nullptr;
  cl::Kernel kernel_;
  cl::Device device_;
};

}  // namespace internal
}  // namespace fpga

#endif  // FPGA_RUNTIME_XILINX_OPENCL_STREAM_H_
