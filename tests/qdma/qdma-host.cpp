#include <cstdlib>

#include <iostream>
#include <thread>

#include "frt.h"

using std::clog;
using std::endl;

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
    c_base[i] = a[i] + b[i];
  }

  fpga::WriteStream a_stream("a");
  fpga::WriteStream b_stream("b");
  fpga::ReadStream c_stream("c");
  auto instance = fpga::Invoke(argv[1], a_stream, b_stream, c_stream);
  const uint64_t kBatchSize = 1ULL << 29;
  auto t1 = std::thread([&]() {
    for (uint64_t i = 0; i < n; i += kBatchSize) {
      a_stream.Write(a + i, std::min(kBatchSize, n - i), !(i + kBatchSize < n));
    }
  });
  auto t2 = std::thread([&]() {
    for (uint64_t i = 0; i < n; i += kBatchSize) {
      b_stream.Write(b + i, std::min(kBatchSize, n - i), !(i + kBatchSize < n));
    }
  });
  auto t3 = std::thread([&]() {
    for (uint64_t i = 0; i < n; i += kBatchSize) {
      c_stream.Read(c + i, std::min(kBatchSize, n - i), !(i + kBatchSize < n));
    }
  });
  t1.join();
  t2.join();
  t3.join();
  instance.Finish();

  clog << "Compute latency: " << instance.ComputeTimeSeconds() << " s" << endl;
  for (int i = 0; i < n; ++i) {
    if (c[i] != c_base[i]) {
      clog << "FAIL: " << c[i] << " != " << c_base[i] << endl;
      return 1;
    }
  }
  clog << "PASS!" << endl;
  free(a);
  free(b);
  free(c);
  delete[] c_base;
  return 0;
}
