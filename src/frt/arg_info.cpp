#include "frt/arg_info.h"

#include <ostream>

namespace fpga {

std::ostream& operator<<(std::ostream& os, const ArgInfo::Cat& cat) {
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

std::ostream& operator<<(std::ostream& os, const ArgInfo& arg) {
  os << "ArgInfo: {index: " << arg.index << ", name: '" << arg.name
     << "', type: '" << arg.type << "', category: " << arg.cat;
  if (!arg.tag.empty()) os << ", tag: '" << arg.tag << "'";
  os << "}";
  return os;
}

}  // namespace fpga
