#include <cstdint>

extern "C" {

void VecAdd(const float* a, const float* b, float* c, uint64_t n) {
#pragma HLS interface m_axi port = a offset = slave bundle = gmem0
#pragma HLS interface m_axi port = b offset = slave bundle = gmem1
#pragma HLS interface m_axi port = c offset = slave bundle = gmem2
#pragma HLS interface s_axilite port = a bundle = control
#pragma HLS interface s_axilite port = b bundle = control
#pragma HLS interface s_axilite port = c bundle = control
#pragma HLS interface s_axilite port = n bundle = control
#pragma HLS interface s_axilite port = return bundle = control
  for (uint64_t i = 0; i < n; ++i) {
#pragma HLS pipeline
    c[i] = a[i] + b[i];
  }
}
}
