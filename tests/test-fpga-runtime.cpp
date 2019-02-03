#include <cstdlib>

#include <iostream>

#include "fpga-runtime.h"

using std::clog;
using std::endl;

extern "C" {
void VecAdd(const float* a, const float* b,
            float* c, uint64_t n);
}

int main(int argc, char* argv[]) {
  uint64_t n = atoi(argv[2]);
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
  fpga::Invoke(argv[1], fpga::ReadOnly(a, n), fpga::ReadOnly(b, n),
               fpga::WriteOnly(c, n), n);
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
