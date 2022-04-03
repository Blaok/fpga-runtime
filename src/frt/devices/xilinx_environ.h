#include <string>
#include <unordered_map>

namespace fpga::xilinx {

using Environ = std::unordered_map<std::string, std::string>;

Environ GetEnviron();

}  // namespace fpga::xilinx
