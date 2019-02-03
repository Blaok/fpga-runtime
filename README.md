# FPGA Runtime

This project provides a runtime for PCIe-based FPGAs programmed under the OpenCL host-kernel model.
For now it works only for Xilinx FPGAs but it should not be hard to support Intel FPGAs as well.

# Prerequisites

+ A C++11 compiler
+ FPGA bitstreams

# Usage

## API

```C++
template <typename... Args> /*...*/ Invoke(const std::string& bitstream, Args&&... args);
```
This invokes the kernel contained in file `bitstream`.
`bitstream` should be a file that can be read via `ifstream` and can be a pipe with proper `EOF`.
`args` are the arguments to the kernel.
If an argument is not a scalar, it needs to be wrapped in one of the following wrappers:

```C++
template <typename T> /*...*/ ReadOnly(T* ptr, size_t n);
template <typename T> /*...*/ WriteOnly(T* ptr, size_t n);
template <typename T> /*...*/ ReadWrite(T* ptr, size_t n);
```
This will tell the runtime the data exchange direction and how many elements are allocated.
The directions are with respect to the kernel device, not the host.
**Passing a host pointer directly will not work (doesn't even compile).**

 `Invoke` returns an `fpga::Instance` object that contains profiling information.
 ```C++
double Instance::LoadTimeSeconds();
double Instance::ComputeTimeSeconds();
double Instance::StoreTimeSeconds();
double Instance::LoadThroughputGbps();
double Instance::StoreThroughputGbps();
 ```
