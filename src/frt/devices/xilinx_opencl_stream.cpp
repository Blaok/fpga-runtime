#include "frt/devices/xilinx_opencl_stream.h"

#include <iostream>
#include <string>

#include <CL/cl2.hpp>

#include "frt/devices/opencl_util.h"

// Link against libxilinxopencl only if necessary.
#pragma weak clCreateStream
#pragma weak clReadStream
#pragma weak clReleaseStream
#pragma weak clWriteStream

namespace fpga {
namespace internal {

XilinxOpenclStream::~XilinxOpenclStream() {
  if (stream_ != nullptr) {
    auto err = clReleaseStream(stream_);
    if (err != CL_SUCCESS) {
      std::clog << "ERROR: clReleaseStream: " << OpenclErrToString(err);
    }
  }
}

XilinxOpenclStream::XilinxOpenclStream(const std::string& name,
                                       cl::Device device, cl::Kernel kernel,
                                       int index, Tag tag)
    : name_(name), kernel_(std::move(kernel)), device_(std::move(device)) {
  cl_stream_flags flags;
  switch (tag) {
    case Tag::kReadOnly:
      flags =
#ifdef XCL_STREAM_WRITE_ONLY
          XCL_STREAM_WRITE_ONLY
#else   // XCL_STREAM_WRITE_ONLY
          CL_STREAM_READ_ONLY
#endif  // XCL_STREAM_WRITE_ONLY
          ;
      break;
    case Tag::kWriteOnly:
      flags =
#ifdef XCL_STREAM_READ_ONLY
          XCL_STREAM_READ_ONLY
#else   // XCL_STREAM_READ_ONLY
          CL_STREAM_WRITE_ONLY
#endif  // XCL_STREAM_READ_ONLY
          ;
      break;
    default:
      throw std::runtime_error("invalid argument");
  }

#ifndef NDEBUG
  std::clog << "DEBUG: Stream '" << name_ << "' attached to argument #" << index
            << std::endl;
#endif
  cl_mem_ext_ptr_t ext;
  ext.flags = index;
  ext.param = kernel_.get();
  ext.obj = nullptr;
  cl_int err;
  stream_ = clCreateStream(device_.get(), flags, CL_STREAM, &ext, &err);
  CL_CHECK(err);
}

void XilinxOpenclStream::Read(void* host_ptr, size_t size, bool eot) {
  if (stream_ == nullptr) {
    throw std::runtime_error("cannot read from null stream");
  }
  cl_stream_xfer_req req{0};
  if (eot) {
    req.flags = CL_STREAM_EOT;
  }
  req.priv_data = const_cast<char*>(name_.c_str());
  cl_int err;
  clReadStream(stream_, host_ptr, size, &req, &err);
  CL_CHECK(err);
}

void XilinxOpenclStream::Write(const void* host_ptr, size_t size, bool eot) {
  if (stream_ == nullptr) {
    throw std::runtime_error("cannot write to null stream");
  }
  cl_stream_xfer_req req{0};
  if (eot) {
    req.flags = CL_STREAM_EOT;
  }
  req.priv_data = const_cast<char*>(name_.c_str());
  cl_int err;
  clWriteStream(stream_, const_cast<void*>(host_ptr), size, &req, &err);
  CL_CHECK(err);
}

}  // namespace internal
}  // namespace fpga
