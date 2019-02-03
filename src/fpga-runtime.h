#ifndef FPGA_RUNTIME_H_
#define FPGA_RUNTIME_H_

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#include <CL/cl2.hpp>

#include "opencl-errors.h"

namespace fpga {

#define CL_CHECK(err) ClCheck((err), __FILE__, __LINE__);
inline void ClCheck(cl_int err, const char* file, int line) {
  if (err != CL_SUCCESS) {
    throw std::runtime_error(
        std::string(file) + ":" + std::to_string(line) + ": " + ToString(err));
  }
}

template <typename T>
class Buffer {
  T* ptr_;
  size_t n_;
 public:
  Buffer(T* ptr, size_t n) : ptr_(ptr), n_(n) {}
  operator T*() const { return ptr_; }
  T& operator*() const { return *ptr_; }
  T* operator->() const { return ptr_; }
  T* Get() const { return ptr_; }
  size_t SizeInBytes() const { return n_ * sizeof(T); }
};
template <typename T>
class RoBuf : public Buffer<T> { using Buffer<T>::Buffer; };
template <typename T>
class WoBuf : public Buffer<T> { using Buffer<T>::Buffer; };
template <typename T>
class RwBuf : public Buffer<T> { using Buffer<T>::Buffer; };

template <typename T>
RoBuf<T> ReadOnly(T* ptr, size_t n) { return RoBuf<T>(ptr, n); }
template <typename T>
WoBuf<T> WriteOnly(T* ptr, size_t n) { return WoBuf<T>(ptr, n); }
template <typename T>
RwBuf<T> ReadWrite(T* ptr, size_t n) { return RwBuf<T>(ptr, n); }

class Instance {
  cl::Context context_;
  cl::CommandQueue cmd_;
  cl::Program program_;
  cl::Kernel kernel_;
  std::unordered_map<int, cl::Memory> buffer_table_;
  std::vector<cl::Memory> load_buffers_;
  std::vector<cl::Memory> store_buffers_;
  std::vector<cl::Event> load_event_;
  std::vector<cl::Event> compute_event_;
  std::vector<cl::Event> store_event_;
 public:
  Instance(const std::string& bitstream);

  // SetArg
  template <typename T>
  void SetArg(int index, const T& arg) { kernel_.setArg(index, arg); }
  template <typename T>
  void SetArg(int index, const RoBuf<T>& arg) {
    kernel_.setArg(index, buffer_table_[index]);
  }
  template <typename T>
  void SetArg(int index, const WoBuf<T>& arg) {
    kernel_.setArg(index, buffer_table_[index]);
  } 
  template <typename T>
  void SetArg(int index, const RwBuf<T>& arg) {
    kernel_.setArg(index, buffer_table_[index]);
  }
  template <typename T, typename... Args>
  void SetArg(int index, const T& arg, const Args&... other_args) {
    SetArg(index, arg);
    SetArg(index + 1, other_args...);
  }
  template <typename... Args>
  void SetArg(const Args&... args) { SetArg(0, args...); }

  // AllocateBuffers
  template <typename T> void AllocateBuffers(int index, const T& arg) {}
  template <typename T>
  void AllocateBuffers(int index, const RoBuf<T>& arg) {
    cl_int err;
    auto buffer = cl::Buffer(context_, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                             arg.SizeInBytes(), arg, &err);
    CL_CHECK(err);
    buffer_table_[index] = buffer;
    load_buffers_.push_back(buffer);
  }
  template <typename T>
  void AllocateBuffers(int index, const WoBuf<T>& arg) {
    cl_int err;
    auto buffer = cl::Buffer(context_, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                             arg.SizeInBytes(), arg, &err);
    CL_CHECK(err);
    buffer_table_[index] = buffer;
    store_buffers_.push_back(buffer);
  }
  template <typename T>
  void AllocateBuffers(int index, const RwBuf<T>& arg) {
    cl_int err;
    auto buffer = cl::Buffer(context_, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                             arg.SizeInBytes(), arg, &err);
    CL_CHECK(err);
    buffer_table_[index] = buffer;
    load_buffers_.push_back(buffer);
    store_buffers_.push_back(buffer);
  }
  template <typename T, typename... Args>
  void AllocateBuffers(int index, const T& arg, const Args&... other_args) {
    AllocateBuffers(index, arg);
    AllocateBuffers(index + 1, other_args...);
  }
  template <typename... Args>
  void AllocateBuffers(const Args&... args) { AllocateBuffers(0, args...); }

  template <typename... Args>
  void WriteToDevice(const Args&... args) {
    load_event_.resize(1);
    CL_CHECK(cmd_.enqueueMigrateMemObjects(
        load_buffers_, /* flags = */ 0, nullptr, load_event_.data()));
  }

  template <typename... Args>
  void ReadFromDevice(const Args&... args) {
    store_buffers_.resize(1);
    CL_CHECK(cmd_.enqueueMigrateMemObjects(
        store_buffers_, CL_MIGRATE_MEM_OBJECT_HOST, &compute_event_,
        store_event_.data()));
  }

  template <typename... Args>
  void Exec(const Args&... args) {
    compute_event_.resize(1);
    CL_CHECK(cmd_.enqueueNDRangeKernel(kernel_, cl::NDRange(1), cl::NDRange(1),
        cl::NDRange(1), &load_event_, compute_event_.data()));
  }

  void Finish() {
    CL_CHECK(cmd_.flush());
    CL_CHECK(cmd_.finish());
  }
};

template <typename F, typename... Args>
void Invoke(const std::string& bitstream, F&& func, Args&&... args) {
  if (bitstream == "") {
    func(args...);
  } else {
    auto instance = Instance(bitstream);
    instance.AllocateBuffers(args...);
    instance.WriteToDevice(args...);
    instance.SetArg(args...);
    instance.Exec(args...);
    instance.ReadFromDevice(args...);
    instance.Finish();
  }
}

#ifndef KEEP_CL_CHECK
#undef CL_CHECK
#endif  // KEEP_CL_CHECK

}   // namespace fpga

#endif  // FPGA_RUNTIME_H_
