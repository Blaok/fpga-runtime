#include "frt.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <CL/cl2.hpp>

#include "frt/intel_opencl_device.h"
#include "frt/tapa_fast_cosim_device.h"
#include "frt/xilinx_opencl_device.h"

namespace fpga {

Instance::Instance(const std::string& bitstream) {
  std::clog << "INFO: Loading " << bitstream << std::endl;
  cl::Program::Binaries binaries;
  {
    std::ifstream stream(bitstream, std::ios::binary);
    binaries = {{std::istreambuf_iterator<char>(stream),
                 std::istreambuf_iterator<char>()}};
  }

  if ((device_ = internal::XilinxOpenclDevice::New(binaries))) {
    return;
  }

  if ((device_ = internal::IntelOpenclDevice::New(binaries))) {
    return;
  }

  if ((device_ = internal::TapaFastCosimDevice::New(
           bitstream,
           std::string_view(reinterpret_cast<char*>(binaries.begin()->data()),
                            binaries.begin()->size())))) {
    return;
  }

  throw std::runtime_error("unexpected bitstream file");
}

size_t Instance::SuspendBuf(int index) { return device_->SuspendBuffer(index); }

void Instance::WriteToDevice() { device_->WriteToDevice(); }

void Instance::ReadFromDevice() { device_->ReadFromDevice(); }

void Instance::Exec() { device_->Exec(); }

void Instance::Finish() { device_->Finish(); }

std::vector<ArgInfo> Instance::GetArgsInfo() const {
  return device_->GetArgsInfo();
}

int64_t Instance::LoadTimeNanoSeconds() {
  return device_->LoadTimeNanoSeconds();
}

int64_t Instance::ComputeTimeNanoSeconds() {
  return device_->ComputeTimeNanoSeconds();
}

int64_t Instance::StoreTimeNanoSeconds() {
  return device_->StoreTimeNanoSeconds();
}

double Instance::LoadTimeSeconds() {
  return static_cast<double>(LoadTimeNanoSeconds()) * 1e-9;
}

double Instance::ComputeTimeSeconds() {
  return static_cast<double>(ComputeTimeNanoSeconds()) * 1e-9;
}

double Instance::StoreTimeSeconds() {
  return static_cast<double>(StoreTimeNanoSeconds()) * 1e-9;
}

double Instance::LoadThroughputGbps() {
  return static_cast<double>(device_->LoadBytes()) /
         static_cast<double>(LoadTimeNanoSeconds());
}

double Instance::StoreThroughputGbps() {
  return static_cast<double>(device_->StoreBytes()) /
         static_cast<double>(StoreTimeNanoSeconds());
}

}  // namespace fpga
