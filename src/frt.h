#ifndef FPGA_RUNTIME_H_
#define FPGA_RUNTIME_H_

#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#include <CL/cl2.hpp>

#include "opencl-errors.h"

extern "C" {
struct _cl_stream;
}  // extern "C"

namespace fpga {

#define CL_CHECK(err) ClCheck((err), __FILE__, __LINE__);
inline void ClCheck(cl_int err, const char* file, int line) {
  if (err != CL_SUCCESS) {
    throw std::runtime_error(std::string(file) + ":" + std::to_string(line) +
                             ": " + ToString(err));
  }
}
#ifdef NDEBUG
#define FUNC_INFO(index)
#else  // NDEBUG
#define FUNC_INFO(index)                                  \
  std::clog << "DEBUG: Function ‘" << __PRETTY_FUNCTION__ \
            << "’ called with index = " << (index) << std::endl;
#endif  // NDEBUG

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
class RoBuf : public Buffer<T> {
  using Buffer<T>::Buffer;
};
template <typename T>
class WoBuf : public Buffer<T> {
  using Buffer<T>::Buffer;
};
template <typename T>
class RwBuf : public Buffer<T> {
  using Buffer<T>::Buffer;
};

template <typename T>
RoBuf<T> ReadOnly(T* ptr, size_t n) {
  return RoBuf<T>(ptr, n);
}
template <typename T>
WoBuf<T> WriteOnly(T* ptr, size_t n) {
  return WoBuf<T>(ptr, n);
}
template <typename T>
RwBuf<T> ReadWrite(T* ptr, size_t n) {
  return RwBuf<T>(ptr, n);
}

class Stream {
 protected:
  const std::string name_;
  _cl_stream* stream_ = nullptr;
  cl::Kernel kernel_;
  cl::Device device_;

  Stream(const std::string& name) : name_(name) {}
  Stream(const Stream&) = delete;
  ~Stream();

  void Attach(const cl::Device& device, const cl::Kernel& kernel, int index,
              /* cl_stream_flags */ uint64_t flags);
};

class ReadStream : public Stream {
 public:
  using Stream::Attach;
  ReadStream(const std::string& name) : Stream(name) {}
  ReadStream(const ReadStream&) = delete;

  void Attach(const cl::Device& device, const cl::Kernel& kernel, int index);

  template <typename T>
  void Read(T* host_ptr, uint64_t size, bool eos = true) {
    ReadRaw(host_ptr, size * sizeof(T), eos);
  }

 private:
  void ReadRaw(void* host_ptr, uint64_t size, bool eos);
};

class WriteStream : public Stream {
 public:
  using Stream::Attach;
  WriteStream(const std::string& name) : Stream(name) {}
  WriteStream(const WriteStream&) = delete;

  void Attach(cl::Device device, cl::Kernel kernel, int index);

  template <typename T>
  void Write(const T* host_ptr, uint64_t size, bool eos = true) {
    WriteRaw(host_ptr, size * sizeof(T), eos);
  }

 private:
  void WriteRaw(const void* host_ptr, uint64_t size, bool eos);
};

struct ArgInfo {
  enum Cat {
    kScalar = 0,
    kMmap = 1,
    kStream = 2,
  };
  int32_t index;
  std::string name;
  std::string type;
  Cat cat;
  std::string tag;
};

inline std::ostream& operator<<(std::ostream& os, const ArgInfo::Cat& cat) {
  switch (cat) {
    case ArgInfo::kScalar:
      return os << "scalar";
    case ArgInfo::kMmap:
      return os << "mmap";
    case ArgInfo::kStream:
      return os << "stream";
  }
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const ArgInfo& arg) {
  os << "ArgInfo: {index: " << arg.index << ", name: ‘" << arg.name
     << "’, type: ‘" << arg.type << "’, category: " << arg.cat;
  if (!arg.tag.empty()) os << ", tag: ‘" << arg.tag << "’";
  os << "}";
  return os;
}

enum class Vendor {
  kUnknown = 0,
  kXilinx = 1,
  kIntel = 2,
};

class Instance {
  cl::Device device_;
  cl::Context context_;
  cl::CommandQueue cmd_;
  cl::Program program_;
  // Maps prefix sum of arg count to kernels.
  std::map<int, cl::Kernel> kernels_;
  std::unordered_map<int, cl::Buffer> buffer_table_;
  std::unordered_map<int, void*> host_ptr_table_;
  std::unordered_map<int32_t, ArgInfo> arg_table_;
  std::vector<int> load_indices_;
  std::vector<int> store_indices_;
  std::vector<cl::Event> load_event_;
  std::vector<cl::Event> compute_event_;
  std::vector<cl::Event> store_event_;
  Vendor vendor_ = Vendor::kUnknown;

  std::vector<cl::Memory> GetLoadBuffers();
  std::vector<cl::Memory> GetStoreBuffers();
  std::pair<int, cl::Kernel> GetKernel(int index) {
    auto it = --this->kernels_.upper_bound(index);
    return {index - it->first, it->second};
  }
  template <typename T>
  void SetArgInternal(int index, T arg) {
    auto pair = this->GetKernel(index);
    pair.second.setArg(pair.first, arg);
  }

 public:
  Instance(const std::string& bitstream);

  // Return info of all args as a vector, sorted by the index.
  std::vector<ArgInfo> GetArgsInfo() {
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

  // SetArg
  template <typename T>
  void SetArg(int index, T&& arg) {
    FUNC_INFO(index)
    this->SetArgInternal(index, arg);
  }
  template <typename T>
  void SetArg(int index, RoBuf<T> arg) {
    FUNC_INFO(index)
    this->SetArgInternal(index, buffer_table_[index]);
  }
  template <typename T>
  void SetArg(int index, WoBuf<T> arg) {
    FUNC_INFO(index)
    this->SetArgInternal(index, buffer_table_[index]);
  }
  template <typename T>
  void SetArg(int index, RwBuf<T> arg) {
    FUNC_INFO(index)
    this->SetArgInternal(index, buffer_table_[index]);
  }
  void SetArg(int index, WriteStream& arg) { FUNC_INFO(index) }
  void SetArg(int index, ReadStream& arg) { FUNC_INFO(index) }
  template <typename T, typename... Args>
  void SetArg(int index, T&& arg, Args&&... other_args) {
    SetArg(index, std::forward<T>(arg));
    SetArg(index + 1, std::forward<Args>(other_args)...);
  }
  template <typename... Args>
  void SetArg(Args&&... args) {
    SetArg(0, std::forward<Args>(args)...);
  }

  cl::Buffer CreateBuffer(int index, cl_mem_flags flags, size_t size,
                          void* host_ptr);

  // AllocBuf
  template <typename T>
  void AllocBuf(int index, T&& arg) {
    FUNC_INFO(index)
  }
  template <typename T>
  void AllocBuf(int index, WoBuf<T> arg) {
    FUNC_INFO(index)
    cl::Buffer buffer = CreateBuffer(
        index, CL_MEM_WRITE_ONLY, arg.SizeInBytes(),
        const_cast<typename std::remove_const<T>::type*>(arg.Get()));
    load_indices_.push_back(index);
  }
  template <typename T>
  void AllocBuf(int index, RoBuf<T> arg) {
    FUNC_INFO(index)
    cl::Buffer buffer =
        CreateBuffer(index, CL_MEM_READ_ONLY, arg.SizeInBytes(), arg.Get());
    store_indices_.push_back(index);
  }
  template <typename T>
  void AllocBuf(int index, RwBuf<T> arg) {
    FUNC_INFO(index)
    cl::Buffer buffer =
        CreateBuffer(index, CL_MEM_READ_WRITE, arg.SizeInBytes(), arg.Get());
    load_indices_.push_back(index);
    store_indices_.push_back(index);
  }
  void AllocBuf(int index, WriteStream& arg) {
    FUNC_INFO(index)
    auto pair = this->GetKernel(index);
    arg.Attach(device_, pair.second, pair.first);
  }
  void AllocBuf(int index, ReadStream& arg) {
    FUNC_INFO(index)
    auto pair = this->GetKernel(index);
    arg.Attach(device_, pair.second, pair.first);
  }

  template <typename T, typename... Args>
  void AllocBuf(int index, T&& arg, Args&&... other_args) {
    AllocBuf(index, std::forward<T>(arg));
    AllocBuf(index + 1, std::forward<Args>(other_args)...);
  }
  template <typename... Args>
  void AllocBuf(Args&&... args) {
    AllocBuf(0, std::forward<Args>(args)...);
  }

  void WriteToDevice();
  void ReadFromDevice();
  void Exec();
  void Finish();

  template <typename... Args>
  Instance& Invoke(Args&&... args) {
    AllocBuf(std::forward<Args>(args)...);
    SetArg(std::forward<Args>(args)...);
    WriteToDevice();
    Exec();
    ReadFromDevice();
    bool has_stream = false;
    bool dummy[sizeof...(Args)]{(
        has_stream |=
        std::is_base_of<Stream,
                        typename std::remove_reference<Args>::type>::value)...};
    if (!has_stream) {
#ifndef NDEBUG
      std::clog << "DEBUG: no stream found; waiting for command to finish"
                << std::endl;
#endif
      Finish();
    }
    return *this;
  }

  cl_ulong LoadTimeNanoSeconds();
  cl_ulong ComputeTimeNanoSeconds();
  cl_ulong StoreTimeNanoSeconds();
  double LoadTimeSeconds();
  double ComputeTimeSeconds();
  double StoreTimeSeconds();
  double LoadThroughputGbps();
  double StoreThroughputGbps();
};

template <typename... Args>
Instance Invoke(const std::string& bitstream, Args&&... args) {
  return Instance(bitstream).Invoke(std::forward<Args>(args)...);
}

#undef FUNC_INFO
#ifndef KEEP_CL_CHECK
#undef CL_CHECK
#endif  // KEEP_CL_CHECK

}  // namespace fpga

#endif  // FPGA_RUNTIME_H_
