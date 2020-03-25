#define KEEP_CL_CHECK
#include "frt.h"

#include <cstring>

#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <elf.h>

#include <tinyxml.h>
#include <CL/cl2.hpp>

#include <CL/cl_ext_xilinx.h>
#include <xclbin.h>

using std::clog;
using std::endl;
using std::runtime_error;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

namespace fpga {

namespace internal {
string Exec(const char* cmd) {
  std::string result;
  unique_ptr<FILE, decltype(&pclose)> pipe{popen(cmd, "r"), pclose};
  if (pipe == nullptr) {
    throw runtime_error(string{"cannot execute: "} + cmd);
  }
  char c;
  while (c = fgetc(pipe.get()), !feof(pipe.get())) {
    result += c;
  }
  return result;
}
}  // namespace internal

cl::Program::Binaries LoadBinaryFile(const string& file_name) {
  clog << "INFO: Loading " << file_name << endl;
  std::ifstream stream(file_name, std::ios::binary);
  return {{std::istreambuf_iterator<char>(stream),
           std::istreambuf_iterator<char>()}};
}

vector<cl::Memory> Instance::GetLoadBuffers() {
  vector<cl::Memory> buffers;
  buffers.reserve(this->load_indices_.size());
  for (auto index : this->load_indices_) {
    buffers.push_back(this->buffer_table_[index]);
  }
  return buffers;
}

vector<cl::Memory> Instance::GetStoreBuffers() {
  vector<cl::Memory> buffers;
  buffers.reserve(this->store_indices_.size());
  for (auto index : this->store_indices_) {
    buffers.push_back(this->buffer_table_[index]);
  }
  return buffers;
}

Instance::Instance(const string& bitstream) {
  cl::Program::Binaries binaries = LoadBinaryFile(bitstream);
  string target_device_name;
  string vendor_name;
  string kernel_name;
  for (const auto& binary : binaries) {
    if (binary.size() >= 8 && memcmp(binary.data(), "xclbin2", 8) == 0) {
      this->vendor_ = Vendor::kXilinx;
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
      target_device_name =
          reinterpret_cast<const char*>(axlf_top->m_header.m_platformVBNV);
      auto metadata = xclbin::get_axlf_section(axlf_top, EMBEDDED_METADATA);
      if (metadata != nullptr) {
        TiXmlDocument doc;
        doc.Parse(
            reinterpret_cast<const char*>(axlf_top) + metadata->m_sectionOffset,
            0, TIXML_ENCODING_UTF8);
        auto xml_core = doc.FirstChildElement("project")
                            ->FirstChildElement("platform")
                            ->FirstChildElement("device")
                            ->FirstChildElement("core");
        auto xml_kernel = xml_core->FirstChildElement("kernel");
        kernel_name = xml_kernel->Attribute("name");
        string target_meta = xml_core->Attribute("target");
        for (auto xml_arg = xml_kernel->FirstChildElement("arg");
             xml_arg != nullptr; xml_arg = xml_arg->NextSiblingElement("arg")) {
          auto index = atoi(xml_arg->Attribute("id"));
          auto& arg = arg_table_[index];
          arg.index = index;
          arg.name = xml_arg->Attribute("name");
          arg.type = xml_arg->Attribute("type");
          auto cat = atoi(xml_arg->Attribute("addressQualifier"));
          switch (cat) {
            case 0:
              arg.cat = ArgInfo::kScalar;
              break;
            case 1:
              arg.cat = ArgInfo::kMmap;
              break;
            case 4:
              arg.cat = ArgInfo::kStream;
              break;
            default:
              clog << "WARNING: Unknown argument category: " << cat;
          }
        }
        // m_mode doesn't always work
        if (target_meta == "hw_em") {
          setenv("XCL_EMULATION_MODE", "hw_emu", 0);
        } else if (target_meta == "csim") {
          setenv("XCL_EMULATION_MODE", "sw_emu", 0);
        }
      } else {
        throw runtime_error("cannot determine kernel name from binary");
      }
      unordered_map<int32_t, string> memory_table;
      auto mem_info = xclbin::get_axlf_section(axlf_top, MEM_TOPOLOGY);
      if (mem_info != nullptr) {
        auto topology = reinterpret_cast<const mem_topology*>(
            reinterpret_cast<const char*>(axlf_top) +
            mem_info->m_sectionOffset);
        for (int i = 0; i < topology->m_count; ++i) {
          const mem_data& mem = topology->m_mem_data[i];
          if (mem.m_used) {
            memory_table[i] = reinterpret_cast<const char*>(mem.m_tag);
          }
        }
      }
      auto conn = xclbin::get_axlf_section(axlf_top, CONNECTIVITY);
      if (conn != nullptr) {
        auto connect = reinterpret_cast<const connectivity*>(
            reinterpret_cast<const char*>(axlf_top) + conn->m_sectionOffset);
        for (int i = 0; i < connect->m_count; ++i) {
          const connection& c = connect->m_connection[i];
          arg_table_[c.arg_index].tag = memory_table[c.mem_data_index];
        }
      }
    } else if (binary.size() >= SELFMAG &&
               memcmp(binary.data(), ELFMAG, SELFMAG) == 0) {
      this->vendor_ = Vendor::kIntel;
      if (binary.data()[EI_CLASS] == ELFCLASS32) {
        vendor_name = "Intel(R) FPGA SDK for OpenCL(TM)";
        auto elf_header = reinterpret_cast<const Elf32_Ehdr*>(binary.data());
        auto elf_section_headers = reinterpret_cast<const Elf32_Shdr*>(
            (reinterpret_cast<const char*>(elf_header) + elf_header->e_shoff));
        auto elf_section = [&](int idx) -> const Elf32_Shdr* {
          return &elf_section_headers[idx];
        };
        auto elf_str_table =
            (elf_header->e_shstrndx == SHN_UNDEF)
                ? nullptr
                : reinterpret_cast<const char*>(elf_header) +
                      elf_section(elf_header->e_shstrndx)->sh_offset;
        auto elf_lookup_string = [&](int offset) -> const char* {
          return elf_str_table ? elf_str_table + offset : nullptr;
        };
        for (int i = 0; i < elf_header->e_shnum; ++i) {
          auto section_header = elf_section(i);
          auto section_name = elf_lookup_string(section_header->sh_name);
          if (strcmp(section_name, ".acl.kernel_arg_info.xml") == 0) {
            TiXmlDocument doc;
            doc.Parse(reinterpret_cast<const char*>(elf_header) +
                          section_header->sh_offset,
                      0, TIXML_ENCODING_UTF8);
            auto xml_kernel =
                doc.FirstChildElement("board")->FirstChildElement("kernel");
            kernel_name = xml_kernel->Attribute("name");
            for (auto xml_arg = xml_kernel->FirstChildElement("argument");
                 xml_arg != nullptr;
                 xml_arg = xml_arg->NextSiblingElement("argument")) {
              auto index = atoi(xml_arg->Attribute("index"));
              auto& arg = this->arg_table_[index];
              arg.index = index;
              arg.name = xml_arg->Attribute("name");
              arg.type = xml_arg->Attribute("type_name");
              auto cat = atoi(xml_arg->Attribute("opencl_access_type"));
              switch (cat) {
                case 0:
                  arg.cat = ArgInfo::kScalar;
                  break;
                case 2:
                  arg.cat = ArgInfo::kMmap;
                  break;
                default:
                  clog << "WARNING: Unknown argument category: " << cat;
              }
            }
          } else if (strcmp(section_name, ".acl.board") == 0) {
            const string board_name(reinterpret_cast<const char*>(elf_header) +
                                        section_header->sh_offset,
                                    section_header->sh_size);
            if (board_name == "EmulatorDevice") {
              setenv("CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA", "1", 0);
            }
            if (board_name == "SimulatorDevice") {
              setenv("CL_CONTEXT_MPSIM_DEVICE_INTELFPGA", "1", 0);
            }
            target_device_name = board_name;
          }
        }
        if (kernel_name.empty() || target_device_name.empty()) {
          throw runtime_error("unexpected ELF file");
        }
      } else if (binary.data()[EI_CLASS] == ELFCLASS64) {
        vendor_name = "Intel(R) FPGA Emulation Platform for OpenCL(TM)";
        target_device_name = "Intel(R) FPGA Emulation Device";
        throw runtime_error("fast emulator not supported");
      } else {
        throw runtime_error("unexpected ELF file");
      }
    } else {
      throw runtime_error("unexpected bitstream file");
    }
  }
  if (getenv("XCL_EMULATION_MODE")) {
    string cmd =
        R"([ "$(jq -r '.Platform.Boards[]|select(.Devices[]|select(.Name==")" +
        target_device_name +
        R"R("))' emconfig.json 2>/dev/null)" != "" ] || )R"
        "emconfigutil --platform " +
        target_device_name;
    if (system(cmd.c_str())) {
      throw std::runtime_error("emconfigutil failed");
    }
    const char* tmpdir = getenv("TMPDIR");
    setenv("SDACCEL_EM_RUN_DIR", tmpdir ? tmpdir : "/tmp", 0);
  }

  const char* xcl_emulation_mode = getenv("XCL_EMULATION_MODE");
  if (xcl_emulation_mode != nullptr && string{"sw_emu"} == xcl_emulation_mode) {
    string ld_library_path;
    if (auto xilinx_sdx = getenv("XILINX_VITIS")) {
      // find LD_LIBRARY_PATH by sourcing ${XILINX_VITIS}/settings64.sh
      ld_library_path = internal::Exec(
          R"(bash -c '. "${XILINX_VITIS}/settings64.sh" && )"
          R"(printf "${LD_LIBRARY_PATH}"')");
    } else {
      // find XILINX_VITIS and LD_LIBRARY_PATH with vivado_hls
      // ld_library_path is null-separated string of both env vars
      ld_library_path = internal::Exec(
          R"(bash -c '. "$(vivado_hls -r -l /dev/null | grep "^/"))"
          R"(/settings64.sh" && printf "${LD_LIBRARY_PATH}\0${XILINX_VITIS}"')");
      setenv("XILINX_VITIS",
             ld_library_path.c_str() + strlen(ld_library_path.c_str()) + 1, 1);
    }
    if (auto xilinx_sdx = getenv("XILINX_SDX")) {
      // find LD_LIBRARY_PATH by sourcing ${XILINX_SDX}/settings64.sh
      ld_library_path = internal::Exec(
          R"(bash -c '. "${XILINX_SDX}/settings64.sh" && )"
          R"(printf "${LD_LIBRARY_PATH}"')");
    } else {
      // find XILINX_SDX and LD_LIBRARY_PATH with vivado_hls
      // ld_library_path is null-separated string of both env vars
      ld_library_path = internal::Exec(
          R"(bash -c '. "$(vivado_hls -r -l /dev/null | grep "^/"))"
          R"(/settings64.sh" && printf "${LD_LIBRARY_PATH}\0${XILINX_SDX}"')");
      setenv("XILINX_SDX",
             ld_library_path.c_str() + strlen(ld_library_path.c_str()) + 1, 1);
    }
    setenv("LD_LIBRARY_PATH", ld_library_path.c_str(), 1);
  }
  vector<cl::Platform> platforms;
  CL_CHECK(cl::Platform::get(&platforms));
  cl_int err;
  for (const auto& platform : platforms) {
    string platformName = platform.getInfo<CL_PLATFORM_NAME>(&err);
    CL_CHECK(err);
    clog << "INFO: Found platform: " << platformName.c_str() << endl;
    if (platformName == vendor_name) {
      vector<cl::Device> devices;
      CL_CHECK(platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices));
      for (const auto& device : devices) {
        const string device_name = device.getInfo<CL_DEVICE_NAME>(&err);
        CL_CHECK(err);
        clog << "INFO: Found device: " << device_name << endl;
        // Intel devices contain a string that is unavailable from the binary.
        bool is_target_device = false;
        switch (this->vendor_) {
          case Vendor::kXilinx: {
            is_target_device = device_name == target_device_name;
            break;
          }
          case Vendor::kIntel: {
            string prefix = target_device_name + " : ";
            is_target_device = device_name.substr(0, prefix.size()) == prefix;
            break;
          }
          default:
            throw runtime_error("unknown vendor");
        }
        if (is_target_device) {
          clog << "INFO: Using " << device_name << endl;
          device_ = device;
          context_ = cl::Context(device, nullptr, nullptr, nullptr, &err);
          if (err == CL_DEVICE_NOT_AVAILABLE) {
            clog << "WARNING: Device ‘" << device_name << "’ not available"
                 << endl;
            continue;
          }
          CL_CHECK(err);
          cmd_ = cl::CommandQueue(context_, device,
                                  CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE |
                                      CL_QUEUE_PROFILING_ENABLE,
                                  &err);
          CL_CHECK(err);
          vector<int> binary_status;
          program_ =
              cl::Program(context_, {device}, binaries, &binary_status, &err);
          for (auto status : binary_status) {
            CL_CHECK(status);
          }
          CL_CHECK(err);
          kernel_ = cl::Kernel(program_, kernel_name.c_str(), &err);
          CL_CHECK(err);
          return;
        }
      }
      throw runtime_error("target device ‘" + target_device_name +
                          "’ not found");
    }
  }
  throw runtime_error("target platform ‘" + vendor_name + "’ not found");
}

cl::Buffer Instance::CreateBuffer(int index, cl_mem_flags flags, size_t size,
                                  void* host_ptr) {
  cl_mem_ext_ptr_t ext;  // must in the same scope as host_ptr
  switch (this->vendor_) {
    case Vendor::kXilinx: {
      flags |= CL_MEM_USE_HOST_PTR;
      if (arg_table_.count(index)) {
        unordered_map<string, decltype(XCL_MEM_DDR_BANK0)> kTagTable{
            {"bank0", XCL_MEM_DDR_BANK0},  {"bank1", XCL_MEM_DDR_BANK1},
            {"bank2", XCL_MEM_DDR_BANK2},  {"bank3", XCL_MEM_DDR_BANK3},
            {"DDR[0]", XCL_MEM_DDR_BANK0}, {"DDR[1]", XCL_MEM_DDR_BANK1},
            {"DDR[2]", XCL_MEM_DDR_BANK2}, {"DDR[3]", XCL_MEM_DDR_BANK3},
        };
        for (int i = 0; i < 32; ++i) {
          kTagTable["HBM[" + std::to_string(i) + "]"] = i | XCL_MEM_TOPOLOGY;
        }
        ext.flags = 0;
        auto flag = kTagTable.find(arg_table_[index].tag);
        if (flag != kTagTable.end()) {
          ext.flags = flag->second;
#ifndef NDEBUG
          clog << "DEBUG: Argument " << index << " assigned to "
               << arg_table_[index].tag << endl;
#endif
        } else if (!arg_table_[index].tag.empty()) {
          clog << "WARNING: Unknown argument memory tag: "
               << arg_table_[index].tag << endl;
        }
        ext.obj = host_ptr;
        ext.param = nullptr;
        flags |= CL_MEM_EXT_PTR_XILINX;
        host_ptr = &ext;
      }
      break;
    }
    case Vendor::kIntel: {
      flags |= /* CL_MEM_HETEROGENEOUS_INTELFPGA = */ 1 << 19;
      host_ptr_table_[index] = host_ptr;
      host_ptr = nullptr;
      break;
    }
    default:
      throw runtime_error("unknown vendor");
  }
  cl_int err;
  auto buffer = cl::Buffer(context_, flags, size, host_ptr, &err);
  CL_CHECK(err);
  buffer_table_[index] = buffer;
  return buffer;
}

void Instance::WriteToDevice() {
  switch (this->vendor_) {
    case Vendor::kXilinx: {
      if (!load_indices_.empty()) {
        load_event_.resize(1);
        CL_CHECK(cmd_.enqueueMigrateMemObjects(
            GetLoadBuffers(), /* flags = */ 0, /* events = */ nullptr,
            load_event_.data()));
      }
      break;
    }
    case Vendor::kIntel: {
      load_event_.resize(this->load_indices_.size());
      for (size_t i = 0; i < this->load_indices_.size(); ++i) {
        auto index = this->load_indices_[i];
        auto buffer = this->buffer_table_[index];
        CL_CHECK(cmd_.enqueueWriteBuffer(
            buffer, /* blocking = */ CL_FALSE, /* offset = */ 0,
            buffer.getInfo<CL_MEM_SIZE>(), host_ptr_table_[index],
            /* events = */ nullptr, &load_event_[i]));
      }
      break;
    }
    default:
      throw runtime_error("unknown vendor");
  }
}

void Instance::ReadFromDevice() {
  switch (this->vendor_) {
    case Vendor::kXilinx: {
      if (!store_indices_.empty()) {
        store_event_.resize(1);
        CL_CHECK(cmd_.enqueueMigrateMemObjects(
            GetStoreBuffers(), CL_MIGRATE_MEM_OBJECT_HOST, &compute_event_,
            store_event_.data()));
      }
      break;
    }
    case Vendor::kIntel: {
      store_event_.resize(this->store_indices_.size());
      for (size_t i = 0; i < this->store_indices_.size(); ++i) {
        auto index = this->store_indices_[i];
        auto buffer = this->buffer_table_[index];
        cmd_.enqueueReadBuffer(buffer, /* blocking = */ CL_FALSE,
                               /* offset = */ 0, buffer.getInfo<CL_MEM_SIZE>(),
                               host_ptr_table_[index], &compute_event_,
                               &store_event_[i]);
      }
      break;
    }
    default:
      throw runtime_error("unknown vendor");
  }
}

void Instance::Exec() {
  compute_event_.resize(1);
  CL_CHECK(cmd_.enqueueNDRangeKernel(kernel_, cl::NullRange, cl::NDRange(1),
                                     cl::NDRange(1), &load_event_,
                                     compute_event_.data()));
}

void Instance::Finish() {
  CL_CHECK(cmd_.flush());
  CL_CHECK(cmd_.finish());
}

template <cl_profiling_info name>
cl_ulong ProfilingInfo(const cl::Event& event) {
  cl_int err;
  auto time = event.getProfilingInfo<name>(&err);
  CL_CHECK(err);
  return time;
}
template <cl_profiling_info name>
cl_ulong Instance::LoadProfilingInfo() {
  if (load_event_.empty()) {
    return 0ULL;
  }
  return ProfilingInfo<name>(load_event_[0]);
}
template <cl_profiling_info name>
cl_ulong Instance::ComputeProfilingInfo() {
  if (compute_event_.empty()) {
    return 0ULL;
  }
  return ProfilingInfo<name>(compute_event_[0]);
}
template <cl_profiling_info name>
cl_ulong Instance::StoreProfilingInfo() {
  if (store_event_.empty()) {
    return 0ULL;
  }
  return ProfilingInfo<name>(store_event_[0]);
}
cl_ulong Instance::LoadTimeNanoSeconds() {
  return LoadProfilingInfo<CL_PROFILING_COMMAND_END>() -
         LoadProfilingInfo<CL_PROFILING_COMMAND_START>();
}
cl_ulong Instance::ComputeTimeNanoSeconds() {
  return ComputeProfilingInfo<CL_PROFILING_COMMAND_END>() -
         ComputeProfilingInfo<CL_PROFILING_COMMAND_START>();
}
cl_ulong Instance::StoreTimeNanoSeconds() {
  return StoreProfilingInfo<CL_PROFILING_COMMAND_END>() -
         StoreProfilingInfo<CL_PROFILING_COMMAND_START>();
}
double Instance::LoadTimeSeconds() { return LoadTimeNanoSeconds() / 1e9; }
double Instance::ComputeTimeSeconds() { return ComputeTimeNanoSeconds() / 1e9; }
double Instance::StoreTimeSeconds() { return StoreTimeNanoSeconds() / 1e9; }
double Instance::LoadThroughputGbps() {
  size_t total_size = 0;
  for (const auto& buffer : GetLoadBuffers()) {
    total_size += buffer.getInfo<CL_MEM_SIZE>();
  }
  return double(total_size) / LoadTimeNanoSeconds();
}
double Instance::StoreThroughputGbps() {
  size_t total_size = 0;
  for (const auto& buffer : GetStoreBuffers()) {
    total_size += buffer.getInfo<CL_MEM_SIZE>();
  }
  return double(total_size) / StoreTimeNanoSeconds();
}
template cl_ulong Instance::LoadProfilingInfo<CL_PROFILING_COMMAND_QUEUED>();
template cl_ulong Instance::LoadProfilingInfo<CL_PROFILING_COMMAND_SUBMIT>();
template cl_ulong Instance::ComputeProfilingInfo<CL_PROFILING_COMMAND_QUEUED>();
template cl_ulong Instance::ComputeProfilingInfo<CL_PROFILING_COMMAND_SUBMIT>();
template cl_ulong Instance::StoreProfilingInfo<CL_PROFILING_COMMAND_QUEUED>();
template cl_ulong Instance::StoreProfilingInfo<CL_PROFILING_COMMAND_SUBMIT>();

}  // namespace fpga
