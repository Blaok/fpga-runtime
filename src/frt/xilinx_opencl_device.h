#ifndef FPGA_RUNTIME_XILINX_OPENCL_DEVICE_H_
#define FPGA_RUNTIME_XILINX_OPENCL_DEVICE_H_

#include <cstddef>

#include <memory>
#include <string>
#include <unordered_map>

#include <CL/cl.h>
#include <CL/cl2.hpp>

#include "frt/opencl_device.h"

namespace fpga {
namespace internal {

class XilinxOpenclDevice : public OpenclDevice {
 public:
  using Environ = std::unordered_map<std::string, std::string>;

  XilinxOpenclDevice(const cl::Program::Binaries& binaries);

  static std::unique_ptr<Device> New(const cl::Program::Binaries& binaries);

  void SetStreamArg(int index, Tag tag, StreamWrapper& arg) override;
  void WriteToDevice() override;
  void ReadFromDevice() override;

  static Environ GetEnviron();

 private:
  cl::Buffer CreateBuffer(int index, cl_mem_flags flags, void* host_ptr,
                          size_t size) override;
};

}  // namespace internal
}  // namespace fpga

#endif  // FPGA_RUNTIME_XILINX_OPENCL_DEVICE_H_
