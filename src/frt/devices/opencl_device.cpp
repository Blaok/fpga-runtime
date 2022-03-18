#include "frt/devices/opencl_device.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <CL/cl2.hpp>

#include "frt/devices/opencl_util.h"

namespace fpga {
namespace internal {

namespace {

template <cl_int name>
int64_t GetTime(const cl::Event& event) {
  cl_int err;
  int64_t time = event.getProfilingInfo<name>(&err);
  CL_CHECK(err);
  return time;
}

template <cl_int name>
int64_t Earliest(const std::vector<cl::Event>& events,
                 int64_t default_value = 0) {
  if (events.size() != 0) {
    default_value = std::numeric_limits<int64_t>::max();
    for (auto& event : events) {
      default_value = std::min(default_value, GetTime<name>(event));
    }
  }
  return default_value;
}

template <cl_int name>
int64_t Latest(const std::vector<cl::Event>& events,
               int64_t default_value = 0) {
  if (events.size() != 0) {
    default_value = std::numeric_limits<int64_t>::min();
    for (auto& event : events) {
      default_value = std::max(default_value, GetTime<name>(event));
    }
  }
  return default_value;
}

}  // namespace

void OpenclDevice::SetScalarArg(int index, const void* arg, int size) {
  auto pair = GetKernel(index);
  pair.second.setArg(pair.first, size, arg);
}

void OpenclDevice::SetBufferArg(int index, Tag tag, const BufferArg& arg) {
  cl_mem_flags flags = 0;
  switch (tag) {
    case Tag::kPlaceHolder:
      break;
    case Tag::kReadOnly:
      flags = CL_MEM_READ_ONLY;
      break;
    case Tag::kWriteOnly:
      flags = CL_MEM_WRITE_ONLY;
      break;
    case Tag::kReadWrite:
      flags = CL_MEM_READ_WRITE;
      break;
  }
  cl::Buffer buffer = CreateBuffer(index, flags, arg.Get(), arg.SizeInBytes());
  if (tag == Tag::kReadOnly || tag == Tag::kReadWrite) {
    store_indices_.insert(index);
  }
  if (tag == Tag::kWriteOnly || tag == Tag::kReadWrite) {
    load_indices_.insert(index);
  }
  auto pair = GetKernel(index);
  pair.second.setArg(pair.first, buffer);
}

size_t OpenclDevice::SuspendBuffer(int index) {
  return load_indices_.erase(index) + store_indices_.erase(index);
}

void OpenclDevice::Exec() {
  compute_event_.resize(kernels_.size());
  int i = 0;
  for (auto& pair : kernels_) {
    CL_CHECK(cmd_.enqueueNDRangeKernel(pair.second, cl::NullRange,
                                       cl::NDRange(1), cl::NDRange(1),
                                       &load_event_, &compute_event_[i]));
    ++i;
  }
}

void OpenclDevice::Finish() {
  CL_CHECK(cmd_.flush());
  CL_CHECK(cmd_.finish());
}

std::vector<ArgInfo> OpenclDevice::GetArgsInfo() const {
  std::vector<ArgInfo> args;
  args.reserve(arg_table_.size());
  for (const auto& arg : arg_table_) {
    args.push_back(arg.second);
  }
  std::sort(args.begin(), args.end(),
            [](const ArgInfo& lhs, const ArgInfo& rhs) {
              return lhs.index < rhs.index;
            });
  return args;
}

int64_t OpenclDevice::LoadTimeNanoSeconds() const {
  return Latest<CL_PROFILING_COMMAND_END>(load_event_) -
         Earliest<CL_PROFILING_COMMAND_START>(load_event_);
}
int64_t OpenclDevice::ComputeTimeNanoSeconds() const {
  return Latest<CL_PROFILING_COMMAND_END>(compute_event_) -
         Earliest<CL_PROFILING_COMMAND_START>(compute_event_);
}
int64_t OpenclDevice::StoreTimeNanoSeconds() const {
  return Latest<CL_PROFILING_COMMAND_END>(store_event_) -
         Earliest<CL_PROFILING_COMMAND_START>(store_event_);
}
size_t OpenclDevice::LoadBytes() const {
  size_t total_size = 0;
  cl_int err;
  for (const auto& buffer : GetLoadBuffers()) {
    total_size += buffer.getInfo<CL_MEM_SIZE>(&err);
    CL_CHECK(err);
  }
  return total_size;
}
size_t OpenclDevice::StoreBytes() const {
  size_t total_size = 0;
  cl_int err;
  for (const auto& buffer : GetStoreBuffers()) {
    total_size += buffer.getInfo<CL_MEM_SIZE>(&err);
    CL_CHECK(err);
  }
  return total_size;
}

void OpenclDevice::Initialize(const cl::Program::Binaries& binaries,
                              const std::string& vendor_name,
                              const std::string& target_device_name,
                              const std::vector<std::string>& kernel_names,
                              const std::vector<int>& kernel_arg_counts) {
  std::vector<cl::Platform> platforms;
  CL_CHECK(cl::Platform::get(&platforms));
  cl_int err;
  for (const auto& platform : platforms) {
    std::string platformName = platform.getInfo<CL_PLATFORM_NAME>(&err);
    CL_CHECK(err);
    std::clog << "INFO: Found platform: " << platformName.c_str() << std::endl;
    if (platformName == vendor_name) {
      std::vector<cl::Device> devices;
      CL_CHECK(platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices));
      for (const auto& device : devices) {
        const std::string device_name = device.getInfo<CL_DEVICE_NAME>(&err);
        CL_CHECK(err);
        std::clog << "INFO: Found device: " << device_name << std::endl;
        // Intel devices contain a std::string that is unavailable from the
        // binary.
        bool is_target_device = false;
        std::string prefix = target_device_name + " : ";
        is_target_device = device_name == target_device_name ||
                           device_name.substr(0, prefix.size()) == prefix;
        if (is_target_device) {
          std::clog << "INFO: Using " << device_name << std::endl;
          device_ = device;
          context_ = cl::Context(device, nullptr, nullptr, nullptr, &err);
          if (err == CL_DEVICE_NOT_AVAILABLE) {
            std::clog << "WARNING: Device '" << device_name << "' not available"
                      << std::endl;
            continue;
          }
          CL_CHECK(err);
          cmd_ = cl::CommandQueue(context_, device,
                                  CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE |
                                      CL_QUEUE_PROFILING_ENABLE,
                                  &err);
          CL_CHECK(err);
          std::vector<int> binary_status;
          program_ =
              cl::Program(context_, {device}, binaries, &binary_status, &err);
          for (auto status : binary_status) {
            CL_CHECK(status);
          }
          CL_CHECK(err);
          CL_CHECK(program_.build());
          for (int i = 0; i < kernel_names.size(); ++i) {
            kernels_[kernel_arg_counts[i]] =
                cl::Kernel(program_, kernel_names[i].c_str(), &err);
            CL_CHECK(err);
          }
          return;
        }
      }
      throw std::runtime_error("target device '" + target_device_name +
                               "' not found");
    }
  }
  throw std::runtime_error("target platform '" + vendor_name + "' not found");
}

cl::Buffer OpenclDevice::CreateBuffer(int index, cl_mem_flags flags,
                                      void* host_ptr, size_t size) {
  cl_int err;
  auto buffer = cl::Buffer(context_, flags, size, host_ptr, &err);
  CL_CHECK(err);
  buffer_table_[index] = buffer;
  return buffer;
}

std::vector<cl::Memory> OpenclDevice::GetLoadBuffers() const {
  std::vector<cl::Memory> buffers;
  buffers.reserve(load_indices_.size());
  for (auto index : load_indices_) {
    buffers.push_back(buffer_table_.at(index));
  }
  return buffers;
}

std::vector<cl::Memory> OpenclDevice::GetStoreBuffers() const {
  std::vector<cl::Memory> buffers;
  buffers.reserve(store_indices_.size());
  for (auto index : store_indices_) {
    buffers.push_back(buffer_table_.at(index));
  }
  return buffers;
}

std::pair<int, cl::Kernel> OpenclDevice::GetKernel(int index) const {
  auto it = std::prev(kernels_.upper_bound(index));
  return {index - it->first, it->second};
}

}  // namespace internal
}  // namespace fpga
