#ifndef FPGA_RUNTIME_INTEL_OPENCL_DEVICE_H_
#define FPGA_RUNTIME_INTEL_OPENCL_DEVICE_H_

#include <cstddef>

#include <memory>
#include <string>

#include <CL/cl.h>
#include <CL/cl2.hpp>

#include "frt/opencl_device.h"

namespace fpga {
namespace internal {

class IntelOpenclDevice : public OpenclDevice {
 public:
  IntelOpenclDevice(const cl::Program::Binaries& binaries);

  static std::unique_ptr<Device> New(const cl::Program::Binaries& binaries);

  void SetStreamArg(int index, Tag tag, StreamWrapper& arg) override;
  void WriteToDevice() override;
  void ReadFromDevice() override;

 private:
  cl::Buffer CreateBuffer(int index, cl_mem_flags flags, void* host_ptr,
                          size_t size) override;

  std::unordered_map<int, void*> host_ptr_table_;
};

}  // namespace internal
}  // namespace fpga

#endif  // FPGA_RUNTIME_INTEL_OPENCL_DEVICE_H_
