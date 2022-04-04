#include <iostream>
#include <string>

#include "frt/devices/xilinx_environ.h"

namespace {

void WriteEnv(const std::string& key, const std::string& value) {
  std::cout.write(key.data(), key.size());
  std::cout.write("=", 1);
  std::cout.write(value.data(), value.size());
  std::cout.write("\0", 1);
}

}  // namespace

int main(int argc, char* argv[]) {
  fpga::xilinx::Environ environ = fpga::xilinx::GetEnviron();

  // If arguments are specified, only print specified environment variables.
  if (argc > 1) {
    for (int i = 1; i < argc; ++i) {
      std::string key = argv[i];
      WriteEnv(key, environ[key]);
    }
    return 0;
  }

  for (const auto& [key, value] : environ) {
    WriteEnv(key, value);
  }
  return 0;
}
