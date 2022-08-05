# FPGA Runtime

This project provides a convenient runtime for PCIe-based FPGAs programmed under the OpenCL host-kernel model.
Both Intel and Xilinx platforms are supported.

## Prerequisites

+ Ubuntu 18.04+

## Install from Binary

```bash
./install.sh
```

## Usage

### Invoking

```C++
template <typename... Args>
fpga::Instance Invoke(const std::string& bitstream, Args&&... args);
```

This invokes the kernel contained in file `bitstream`.
`bitstream` should be a file that can be read via `ifstream` and can be a pipe with proper `EOF`.
`args` are the arguments to the kernel.
If an argument is not a scalar, it needs to be wrapped in one of the following wrappers:

```C++
ReadOnly(T* ptr, size_t n);
WriteOnly(T* ptr, size_t n);
ReadWrite(T* ptr, size_t n);
```

This will tell the runtime the data exchange direction and how many elements are allocated.
The directions are with respect to the host, not the device (because *this is host code*).
**Passing a host pointer directly will not work (doesn't even compile).**

### Device Selection

By default, FRT selects devices using metadata from the `bitstream`.
This may not always work as expected, often due to the following reasons:

1. Xilinx [2RP](https://docs.xilinx.com/r/en-US/ug1301-getting-started-guide-alveo-accelerator-cards/Programming-the-Shell-Partition-for-DFX-2RP-Platforms) shell platforms must be flashed by admin (`root`) before running any user logic.
2. FRT may not know how to match the device name in the `bitstream` and the runtime device name. If you encounter this issue, please feel free to [file a bug](https://github.com/Blaok/fpga-runtime/issues/new).

#### Selecting Xilinx Device by PCIe BDF

For Xilinx devices, it is possible to select the device by its
[PCIe BDF](https://docs.xilinx.com/r/en-US/ug1531-vck5000-install/Obtaining-Card-BDF-Values).

To do this, make sure you parsed `gflags` in your main function:

```C++
#include <gflags/gflags.h>
...
int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, /*remove_flags=*/true);
  ...
}
```

When running the host program, add `--xocl_bdf=<bdf>`, e.g.,

```Bash
./host --xocl_bdf=0000:d8:00.1 ...
```

### Profiling

`Invoke` returns an `fpga::Instance` object that contains profiling information.

```C++
double Instance::LoadTimeSeconds();
double Instance::ComputeTimeSeconds();
double Instance::StoreTimeSeconds();
double Instance::LoadThroughputGbps();
double Instance::StoreThroughputGbps();
```

### Streaming

Streaming is supported (on legacy Xilinx platforms).

```C++
class fpga::ReadStream;
class fpga::WriteStream;
```

The streams need to be created and passed to `fpga::Invoke` as a parameter.
If the arguments to `fpga::Invoke` contains a stream,
  it will not wait for the kernel to finish;
  instead, it will return an `fpga::Instance` object immediately.
The host program can read from `fpga::ReadStream` and/or write to
  `fpga::WriteStream`.
When all stream I/O are done,
  `instance.Finish()` should be invoked to wait until the kernel finishes.
