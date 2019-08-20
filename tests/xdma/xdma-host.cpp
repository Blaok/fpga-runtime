#include <cstdlib>

#include <iostream>

#include "frt.h"

using std::clog;
using std::endl;

extern "C" {
void VecAdd(const float* a, const float* b, float* c, uint64_t n);
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    clog << "Usage: " << argv[0] << " <bitstream> <n>" << endl;
    return 1;
  }
  uint64_t n = (atoi(argv[2]) / 1024 + 1) * 1024;
  auto a = reinterpret_cast<float*>(aligned_alloc(4096, sizeof(float) * n));
  auto b = reinterpret_cast<float*>(aligned_alloc(4096, sizeof(float) * n));
  auto c = reinterpret_cast<float*>(aligned_alloc(4096, sizeof(float) * n));
  auto c_base = new float[n];
  for (int i = 0; i < n; ++i) {
    a[i] = i * i % 10;
    b[i] = i * i % 9;
    c[i] = 0;
    c_base[i] = 1;
  }
  auto instance = fpga::Invoke(argv[1], fpga::WriteOnly(a, n),
                               fpga::WriteOnly(b, n), fpga::ReadOnly(c, n), n);
  clog << "Load throughput: " << instance.LoadThroughputGbps() << " GB/s\n";
  clog << "Compute latency: " << instance.ComputeTimeSeconds() << " s" << endl;
  clog << "Store throughput: " << instance.StoreThroughputGbps() << " GB/s\n";
  VecAdd(a, b, c_base, n);
  for (int i = 0; i < n; ++i) {
    if (c[i] != c_base[i]) {
      clog << "FAIL: " << c[i] << " != " << c_base[i] << endl;
      return 1;
    }
  }
  clog << "PASS!" << endl;
  delete[] a;
  delete[] b;
  delete[] c;
  delete[] c_base;
  return 0;
}
