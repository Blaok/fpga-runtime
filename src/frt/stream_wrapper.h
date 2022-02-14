#ifndef FPGA_RUNTIME_STREAM_WRAPPER_H_
#define FPGA_RUNTIME_STREAM_WRAPPER_H_

#include <memory>
#include <string>

#include "frt/stream_interface.h"

namespace fpga {
namespace internal {

class StreamWrapper {
 public:
  void Attach(std::unique_ptr<StreamInterface>&& stream) {
    stream_ = std::move(stream);
  }
  const std::string name;

 protected:
  StreamWrapper(const std::string& name) : name(name) {}
  std::unique_ptr<StreamInterface> stream_;
};

}  // namespace internal
}  // namespace fpga

#endif  // FPGA_RUNTIME_STREAM_WRAPPER_H_
