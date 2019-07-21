#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>

using pkt = qdma_axis<32, 0, 0, 0>;

extern "C" {

void VecAdd(hls::stream<pkt>& a, hls::stream<pkt>& b, hls::stream<pkt>& c) {
#pragma HLS interface axis port = a
#pragma HLS interface axis port = b
#pragma HLS interface axis port = c
#pragma HLS interface s_axilite port = return bundle = control
  for (bool eos = false; !eos;) {
#pragma HLS pipeline
    pkt a_pkt = a.read();
    pkt b_pkt = b.read();
    ap_uint<32> a_raw = a_pkt.get_data();
    ap_uint<32> b_raw = b_pkt.get_data();
    eos = a_pkt.get_last() | b_pkt.get_last();
    float a_val = reinterpret_cast<const float&>(a_raw);
    float b_val = reinterpret_cast<const float&>(b_raw);

    float c_val = a_val + b_val;
    ap_uint<32> c_raw = reinterpret_cast<const ap_uint<32>&>(c_val);

    pkt c_pkt;
    c_pkt.set_data(c_raw);
    c_pkt.set_last(eos);
    c_pkt.set_keep(-1);
    c.write(c_pkt);
  }
}
}
