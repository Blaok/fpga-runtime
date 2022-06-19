#ifndef FPGA_RUNTIME_TAPA_FAST_COSIM_
#define FPGA_RUNTIME_TAPA_FAST_COSIM_

#include <chrono>
#include <ratio>
#include <string>
#include <string_view>
#include <unordered_map>

#include <CL/cl2.hpp>
#include <unordered_set>

#include "frt/buffer.h"
#include "frt/device.h"

namespace fpga {
namespace internal {

class TapaFastCosimDevice : public Device {
 public:
  TapaFastCosimDevice(std::string_view bitstream);
  TapaFastCosimDevice(const TapaFastCosimDevice&) = delete;
  TapaFastCosimDevice& operator=(const TapaFastCosimDevice&) = delete;
  TapaFastCosimDevice(TapaFastCosimDevice&&) = delete;
  TapaFastCosimDevice& operator=(TapaFastCosimDevice&&) = delete;

  ~TapaFastCosimDevice() override;

  static std::unique_ptr<Device> New(std::string_view path,
                                     std::string_view content);

  void SetScalarArg(int index, const void* arg, int size) override;
  void SetBufferArg(int index, Tag tag, const BufferArg& arg) override;
  void SetStreamArg(int index, Tag tag, StreamWrapper& arg) override;
  size_t SuspendBuffer(int index) override;

  void WriteToDevice() override;
  void ReadFromDevice() override;
  void Exec() override;
  void Finish() override;

  std::vector<ArgInfo> GetArgsInfo() const override;
  int64_t LoadTimeNanoSeconds() const override;
  int64_t ComputeTimeNanoSeconds() const override;
  int64_t StoreTimeNanoSeconds() const override;
  size_t LoadBytes() const override;
  size_t StoreBytes() const override;

  const std::string xo_path;
  const std::string work_dir;

 private:
  std::unordered_map<int, std::string> scalars_;
  std::unordered_map<int, BufferArg> buffer_table_;
  std::vector<ArgInfo> args_;
  std::unordered_set<int> load_indices_;
  std::unordered_set<int> store_indices_;

  std::chrono::nanoseconds load_time_;
  std::chrono::nanoseconds compute_time_;
  std::chrono::nanoseconds store_time_;
};

}  // namespace internal
}  // namespace fpga

#endif  // FPGA_RUNTIME_TAPA_FAST_COSIM_
