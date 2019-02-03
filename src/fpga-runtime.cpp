#define KEEP_CL_CHECK
#include "fpga-runtime.h"

#include <cstring>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <CL/cl2.hpp>
#include <xclbin.h>
#include <tinyxml.h>

using std::clog;
using std::endl;
using std::runtime_error;
using std::string;
using std::vector;

namespace fpga {

cl::Program::Binaries LoadBinaryFile(const string& file_name) {
  clog << "INFO: Loading " << file_name << endl;
  std::ifstream stream(file_name.c_str(), std::ios::binary);
  vector<unsigned char> contents((std::istreambuf_iterator<char>(stream)),
                                 std::istreambuf_iterator<char>());
  return {contents};
}

Instance::Instance(const string& bitstream) {
  cl::Program::Binaries binaries = LoadBinaryFile(bitstream);
  string target_device_name;
  string vendor_name;
  string kernel_name;
  for (const auto& binary : binaries) {
    if (memcmp(binary.data(), "xclbin2", 8) == 0) {
      vendor_name = "Xilinx";
      const auto axlf_top = reinterpret_cast<const axlf*>(binary.data());
      switch (axlf_top->m_header.m_mode) {
        case XCLBIN_FLAT:
        case XCLBIN_PR:
        case XCLBIN_TANDEM_STAGE2:
        case XCLBIN_TANDEM_STAGE2_WITH_PR:
          break;
        case XCLBIN_HW_EMU:
          setenv("XCL_EMULATION_MODE", "hw_emu", 0);
          break;
        case XCLBIN_SW_EMU:
          setenv("XCL_EMULATION_MODE", "sw_emu", 0);
          break;
        default:
          throw runtime_error("unknown xclbin mode");
      }
      target_device_name = reinterpret_cast<const char*>(
          axlf_top->m_header.m_platformVBNV);
      auto metadata = xclbin::get_axlf_section(axlf_top, EMBEDDED_METADATA);
      if (metadata != nullptr) {
        TiXmlDocument doc;
        doc.Parse(reinterpret_cast<const char*>(axlf_top) +
                  metadata->m_sectionOffset, 0, TIXML_ENCODING_UTF8);
        kernel_name = doc.FirstChildElement("project")
          ->FirstChildElement("platform")->FirstChildElement("device")
          ->FirstChildElement("core")->FirstChildElement("kernel")
          ->Attribute("name");
        string target_meta = doc.FirstChildElement("project")
          ->FirstChildElement("platform")->FirstChildElement("device")
          ->FirstChildElement("core")->Attribute("target");
        // m_mode doesn't always work
        if (target_meta == "hw_em") {
          setenv("XCL_EMULATION_MODE", "hw_emu", 0);
        } else if (target_meta == "csim") {
          setenv("XCL_EMULATION_MODE", "sw_emu", 0);
        }
      } else {
        throw runtime_error("cannot determine kernel name from binary");
      }
    } else {
      throw runtime_error("unknown bitstream file");
    }
  }
  vector<cl::Platform> platforms;
  CL_CHECK(cl::Platform::get(&platforms));
  cl_int err;
  for (const auto& platform : platforms) {
    string platformName = platform.getInfo<CL_PLATFORM_NAME>(&err);
    CL_CHECK(err);
    clog << "INFO: Found platform: " << platformName.c_str() << endl;
    if (platformName == vendor_name){
      vector<cl::Device> devices;
      CL_CHECK(platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices));
      for (const auto& device : devices) {
        const string device_name = device.getInfo<CL_DEVICE_NAME>(&err);
        CL_CHECK(err);
        clog << "INFO: Found device: " << device_name << endl;
        if (device_name == target_device_name) {
          clog << "INFO: Using " << device_name << endl;
          context_ = cl::Context(device, nullptr, nullptr, nullptr, &err);
          if (err == CL_DEVICE_NOT_AVAILABLE) {
            continue;
          }
          CL_CHECK(err);
          cmd_ = cl::CommandQueue(
              context_, device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
          CL_CHECK(err);
          vector<int> binary_status;
          program_ = cl::Program(
              context_, {device}, binaries, &binary_status, &err);
          for (auto status : binary_status) {
            CL_CHECK(status);
          }
          CL_CHECK(err);
          kernel_ = cl::Kernel(program_, kernel_name.c_str(), &err);
          CL_CHECK(err);
          return;
        }
      }
    }
  }
}

}   // namespace fpga
