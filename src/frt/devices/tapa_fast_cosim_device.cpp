#include "frt/devices/tapa_fast_cosim_device.h"

#include <cstdlib>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <tinyxml.h>
#include <unistd.h>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <nlohmann/json.hpp>
#include <subprocess.hpp>
#include <zip_file.hpp>

#include "frt/arg_info.h"
#include "frt/devices/xilinx_environ.h"

#ifdef __cpp_lib_filesystem
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

DEFINE_bool(xosim_start_gui, false, "start Vivado GUI for simulation");
DEFINE_bool(xosim_save_waveform, false, "save waveform in the work directory");
DEFINE_string(xosim_work_dir, "",
              "if not empty, use the specified work directory instead of a "
              "temporary one");

namespace fpga {
namespace internal {

namespace {

using clock = std::chrono::steady_clock;

std::string GetWorkDirectory() {
  if (!FLAGS_xosim_work_dir.empty()) {
    LOG_IF(INFO, fs::create_directories(FLAGS_xosim_work_dir))
        << "created work directory '" << FLAGS_xosim_work_dir << "'";
    return fs::absolute(FLAGS_xosim_work_dir).string();
  }
  std::string dir =
      (fs::temp_directory_path() / "tapa-fast-cosim.XXXXXX").string();
  LOG_IF(FATAL, ::mkdtemp(&dir[0]) == nullptr)
      << "failed to create work directory";
  return dir;
}

std::string GetInputDataPath(const std::string& work_dir, int index) {
  return work_dir + "/" + std::to_string(index) + ".bin";
}

std::string GetOutputDataPath(const std::string& work_dir, int index) {
  return work_dir + "/" + std::to_string(index) + "_out.bin";
}

std::string GetConfigPath(const std::string& work_dir) {
  return work_dir + "/config.json";
}

}  // namespace

TapaFastCosimDevice::TapaFastCosimDevice(std::string_view xo_path)
    : xo_path(fs::absolute(xo_path)), work_dir(GetWorkDirectory()) {
  miniz_cpp::zip_file xo_file = this->xo_path;
  std::string kernel_xml;
  for (auto& info : xo_file.infolist()) {
    constexpr std::string_view kSuffix = "/kernel.xml";
    if (info.filename.size() >= kSuffix.size() &&
        std::equal(kSuffix.rbegin(), kSuffix.rend(), info.filename.rbegin())) {
      kernel_xml = xo_file.read(info);
      break;
    }
  }
  LOG_IF(FATAL, kernel_xml.empty())
      << "Missing 'kernel.xml' in '" << xo_path << "'";

  TiXmlDocument doc;
  doc.Parse(kernel_xml.data(), nullptr, TIXML_ENCODING_UTF8);
  for (const TiXmlElement* xml_arg = doc.FirstChildElement("root")
                                         ->FirstChildElement("kernel")
                                         ->FirstChildElement("args")
                                         ->FirstChildElement("arg");
       xml_arg != nullptr; xml_arg = xml_arg->NextSiblingElement("arg")) {
    ArgInfo arg;
    arg.index = atoi(xml_arg->Attribute("id"));
    LOG_IF(FATAL, arg.index != args_.size())
        << "Expecting argument #" << args_.size() << ", got argument #"
        << arg.index << " in the metadata";
    arg.name = xml_arg->Attribute("name");
    arg.type = xml_arg->Attribute("type");
    switch (int cat = atoi(xml_arg->Attribute("addressQualifier")); cat) {
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
        LOG(WARNING) << "Unknown argument category: " << cat;
    }
    args_.push_back(arg);
  }

  LOG(INFO) << "Running hardware simulation with TAPA fast cosim";
}

TapaFastCosimDevice::~TapaFastCosimDevice() {
  if (FLAGS_xosim_work_dir.empty()) {
    fs::remove_all(work_dir);
  }
}

std::unique_ptr<Device> TapaFastCosimDevice::New(std::string_view path,
                                                 std::string_view content) {
  constexpr std::string_view kZipMagic("PK\3\4", 4);
  if (content.size() < kZipMagic.size() ||
      memcmp(content.data(), kZipMagic.data(), kZipMagic.size()) != 0) {
    return nullptr;
  }
  return std::make_unique<TapaFastCosimDevice>(path);
}

void TapaFastCosimDevice::SetScalarArg(int index, const void* arg, int size) {
  LOG_IF(FATAL, index >= args_.size())
      << "Cannot set argument #" << index << "; there are only " << args_.size()
      << " arguments";
  LOG_IF(FATAL, args_[index].cat != ArgInfo::kScalar)
      << "Cannot set argument '" << args_[index].name
      << "' as a scalar; it is a " << args_[index].cat;
  std::basic_string_view<unsigned char> arg_str(
      reinterpret_cast<const unsigned char*>(arg), size);
  std::stringstream ss;
  ss << "'h";
  // Assuming litten-endian.
  for (auto it = arg_str.crbegin(); it < arg_str.crend(); ++it) {
    ss << std::setfill('0') << std::setw(2) << std::hex << int(*it);
  }
  scalars_[index] = ss.str();
}

void TapaFastCosimDevice::SetBufferArg(int index, Tag tag,
                                       const BufferArg& arg) {
  LOG_IF(FATAL, index >= args_.size())
      << "Cannot set argument #" << index << "; there are only " << args_.size()
      << " arguments";
  LOG_IF(FATAL, args_[index].cat != ArgInfo::kMmap)
      << "Cannot set argument '" << args_[index].name
      << "' as an mmap; it is a " << args_[index].cat;
  buffer_table_.insert({index, arg});
  if (tag == Tag::kReadOnly || tag == Tag::kReadWrite) {
    store_indices_.insert(index);
  }
  if (tag == Tag::kWriteOnly || tag == Tag::kReadWrite) {
    load_indices_.insert(index);
  }
}

void TapaFastCosimDevice::SetStreamArg(int index, Tag tag, StreamWrapper& arg) {
  LOG(FATAL) << "TAPA fast cosim device does not support streaming";
}

size_t TapaFastCosimDevice::SuspendBuffer(int index) {
  return load_indices_.erase(index) + store_indices_.erase(index);
}

void TapaFastCosimDevice::WriteToDevice() {
  // All buffers must have a data file.
  auto tic = clock::now();
  for (const auto& [index, buffer_arg] : buffer_table_) {
    std::ofstream(GetInputDataPath(work_dir, index),
                  std::ios::out | std::ios::binary)
        .write(buffer_arg.Get(), buffer_arg.SizeInBytes());
  }
  load_time_ = clock::now() - tic;
}

void TapaFastCosimDevice::ReadFromDevice() {
  auto tic = clock::now();
  for (int index : store_indices_) {
    auto buffer_arg = buffer_table_.at(index);
    std::ifstream(GetOutputDataPath(work_dir, index),
                  std::ios::in | std::ios::binary)
        .read(buffer_arg.Get(), buffer_arg.SizeInBytes());
  }
  store_time_ = clock::now() - tic;
}

void TapaFastCosimDevice::Exec() {
  auto tic = clock::now();

  nlohmann::json json;
  json["xo_path"] = xo_path;
  auto& scalar_to_val = json["scalar_to_val"];
  for (const auto& [index, scalar] : scalars_) {
    scalar_to_val[std::to_string(index)] = scalar;
  }
  auto& axi_to_c_array_size = json["axi_to_c_array_size"];
  auto& axi_to_data_file = json["axi_to_data_file"];
  for (const auto& [index, content] : buffer_table_) {
    axi_to_c_array_size[std::to_string(index)] = content.SizeInCount();
    axi_to_data_file[std::to_string(index)] = GetInputDataPath(work_dir, index);
  }
  std::ofstream(GetConfigPath(work_dir)) << json.dump(2);

  std::vector<std::string> argv = {
      "python3",
      "-m",
      "tapa_fast_cosim.main",
      "--config_path=" + GetConfigPath(work_dir),
      "--tb_output_dir=" + work_dir + "/output",
      "--launch_simulation",
  };
  if (FLAGS_xosim_start_gui) {
    argv.push_back("--start_gui");
  }
  if (FLAGS_xosim_save_waveform) {
    argv.push_back("--save_waveform");
  }
  int rc =
      subprocess::Popen(argv, subprocess::environment(xilinx::GetEnviron()))
          .wait();
  LOG_IF(FATAL, rc != 0) << "TAPA fast cosim failed";

  compute_time_ = clock::now() - tic;
}

void TapaFastCosimDevice::Finish() {
  // Not implemented.
}

std::vector<ArgInfo> TapaFastCosimDevice::GetArgsInfo() const { return args_; }

int64_t TapaFastCosimDevice::LoadTimeNanoSeconds() const {
  return load_time_.count();
}

int64_t TapaFastCosimDevice::ComputeTimeNanoSeconds() const {
  return compute_time_.count();
}

int64_t TapaFastCosimDevice::StoreTimeNanoSeconds() const {
  return store_time_.count();
}

size_t TapaFastCosimDevice::LoadBytes() const {
  size_t total_size = 0;
  for (auto& [index, buffer_arg] : buffer_table_) {
    total_size += buffer_arg.SizeInBytes();
  }
  return total_size;
}

size_t TapaFastCosimDevice::StoreBytes() const {
  size_t total_size = 0;
  for (int index : store_indices_) {
    auto buffer_arg = buffer_table_.at(index);
    total_size += buffer_arg.SizeInBytes();
  }
  return total_size;
}

}  // namespace internal
}  // namespace fpga
