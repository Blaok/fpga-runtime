cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)

project(frt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(APPEND CMAKE_PROGRAM_PATH "${CMAKE_CURRENT_LIST_DIR}/src")
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(gflags REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenCL REQUIRED)
find_package(TinyXML REQUIRED)
find_package(XRT REQUIRED)

set(DEFAULT_ENABLE_XOCL_STREAM ON)
if(XRT_VERSION VERSION_GREATER_EQUAL 2.13)
  set(DEFAULT_ENABLE_XOCL_STREAM OFF)
endif()
option(
  ENABLE_XOCL_STREAM
  "Enable Xilinx streaming APIs"
  ${DEFAULT_ENABLE_XOCL_STREAM}
)

include_directories(SYSTEM ${XRT_INCLUDE_DIR})

file(
  DOWNLOAD
  https://github.com/nlohmann/json/releases/download/v3.10.5/json.hpp
  ${CMAKE_CURRENT_BINARY_DIR}/include/nlohmann/json.hpp
)
file(
  DOWNLOAD
  https://github.com/arun11299/cpp-subprocess/raw/v2.0/subprocess.hpp
  ${CMAKE_CURRENT_BINARY_DIR}/include/subprocess.hpp
  TLS_VERIFY ON
)
execute_process(
  COMMAND
    sed -i "-es/map/unordered_map/"
    "-es/std::initializer_list<const char\\*>/std::initializer_list<std::string_view>/"
    "-es/waitpid(pid, \\&status, WNOHANG)/waitpid(pid, \\&status, 0)/"
    ${CMAKE_CURRENT_BINARY_DIR}/include/subprocess.hpp
)

file(
  DOWNLOAD
  https://github.com/tfussell/miniz-cpp/raw/052335e4f7/zip_file.hpp
  ${CMAKE_CURRENT_BINARY_DIR}/include/zip_file.hpp
  TLS_VERIFY ON
)
execute_process(
  COMMAND
    sed -i "-es/std::uint64_t/mz_uint64/"
    ${CMAKE_CURRENT_BINARY_DIR}/include/zip_file.hpp
)

set(frt_sources
    src/frt.cpp
    src/frt/arg_info.cpp
    src/frt/devices/intel_opencl_device.cpp
    src/frt/devices/opencl_device.cpp
    src/frt/devices/tapa_fast_cosim_device.cpp
    src/frt/devices/xilinx_environ.cpp
    src/frt/devices/xilinx_opencl_device.cpp
)
set(frt_compile_features
    cxx_std_17
    cxx_alias_templates
    cxx_auto_type
    cxx_decltype
    cxx_defaulted_functions
    cxx_nullptr
    cxx_override
    cxx_range_for
    cxx_right_angle_brackets
    cxx_rvalue_references
    cxx_strong_enums
    cxx_variadic_templates
)
set(frt_compile_definitions
    CL_HPP_CL_1_2_DEFAULT_BUILD
    CL_HPP_TARGET_OPENCL_VERSION=120
    CL_HPP_MINIMUM_OPENCL_VERSION=120
    CL_TARGET_OPENCL_VERSION=120
    CL_MINIMUM_OPENCL_VERSION=120
)
if(ENABLE_XOCL_STREAM)
  message(STATUS "Building FRT with Xilinx streaming APIs")
  list(APPEND frt_sources src/frt/devices/xilinx_opencl_stream.cpp)
  list(APPEND frt_compile_definitions FRT_ENABLE_XOCL_STREAM)
endif()

set(frt_private_link_libraries
    -l:libOpenCL.so.1 # Nvidia's OpenCL doesn't have libOpenCL.so
    gflags
    glog
    stdc++fs
    tinyxml
)
set(frt_public_link_libraries Threads::Threads)

add_library(frt_static STATIC)
add_library(frt ALIAS frt_static)
add_library(frt_shared SHARED)
target_sources(frt_static PRIVATE ${frt_sources})
target_sources(frt_shared PRIVATE ${frt_sources})
target_compile_features(frt_static PUBLIC ${frt_compile_features})
target_compile_features(frt_shared PUBLIC ${frt_compile_features})
target_compile_definitions(frt_static PRIVATE ${frt_compile_definitions})
target_compile_definitions(frt_shared PRIVATE ${frt_compile_definitions})
set_target_properties(frt_static frt_shared PROPERTIES OUTPUT_NAME frt)
set_target_properties(frt_shared PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(frt_static PROPERTIES EXPORT_NAME frt)
target_include_directories(
  frt_static
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include
)
target_include_directories(
  frt_shared
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include
)
target_link_libraries(
  frt_static
  PRIVATE ${frt_private_link_libraries}
  PUBLIC ${frt_public_link_libraries}
)
target_link_libraries(
  frt_shared
  PRIVATE ${frt_private_link_libraries}
  PUBLIC ${frt_public_link_libraries}
)

add_executable(frt_get_xlnx_env)
target_sources(
  frt_get_xlnx_env
  PRIVATE src/frt/devices/xilinx_environ.cpp src/frt_get_xlnx_env.cpp
)
target_compile_features(frt_get_xlnx_env PRIVATE cxx_std_17)
target_include_directories(
  frt_get_xlnx_env PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include
)
target_link_libraries(
  frt_get_xlnx_env PRIVATE Threads::Threads -static-libgcc -static-libstdc++
)

include(GNUInstallDirs)
install(
  TARGETS frt_static frt_shared frt_get_xlnx_env
  EXPORT FRTTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(
  DIRECTORY "${CMAKE_SOURCE_DIR}/src/"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING
  PATTERN "devices" EXCLUDE
  PATTERN "*.h"
)
install(
  PROGRAMS "${CMAKE_SOURCE_DIR}/src/frt_vpp_wrapper"
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

export(
  EXPORT FRTTargets
  NAMESPACE frt::
  FILE ${CMAKE_CURRENT_BINARY_DIR}/frt/FRTTargets.cmake
)
set(ConfigPackageLocation lib/cmake/frt)
install(
  EXPORT FRTTargets
  FILE FRTTargets.cmake
  NAMESPACE frt::
  DESTINATION ${ConfigPackageLocation}
)
install(
  FILES cmake/FRTConfig.cmake
  DESTINATION ${ConfigPackageLocation}
  COMPONENT Devel
)
install(
  FILES cmake/FindXRT.cmake
  RENAME XRTConfig.cmake
  DESTINATION ${ConfigPackageLocation}
  COMPONENT Devel
)
install(
  FILES cmake/FindSDx.cmake
  RENAME SDxConfig.cmake
  DESTINATION ${ConfigPackageLocation}
  COMPONENT Devel
)

set(CPACK_PACKAGE_NAME libfrt-dev)
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 20230903.1)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "FPGA Runtime for PCIe-Based FPGAs")

string(
  JOIN
  ", "
  CPACK_DEBIAN_PACKAGE_DEPENDS
  "bash"
  "coreutils"
  "fuse" # for `fusermount`
  "libgflags-dev"
  "libgoogle-glog-dev"
  "libtinyxml-dev"
  "ocl-icd-libopencl1 | libopencl1"
  "procps" # for `ps`
  "util-linux" # for `setsid`
)
string(
  JOIN
  ", "
  CPACK_DEBIAN_PACKAGE_RECOMMENDS
  "bindfs"
  "csh"
  "squashfs-tools"
  "squashfuse"
  "xrt"
)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Blaok Chi")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

string(
  JOIN
  ", "
  CPACK_RPM_PACKAGE_REQUIRES
  "bash"
  "coreutils"
  "fuse" # for `fusermount`
  "gflags"
  "glog"
  "ocl-icd-devel"
  "procps-ng" # for `ps`
  "tinyxml-devel"
  "util-linux" # for `setsid`
)
set(CPACK_RPM_PACKAGE_NAME frt-devel)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

set(CPACK_GENERATOR
    TGZ DEB
    CACHE STRING "Enable selected CPack generators"
)
include(CPack)

enable_testing()

find_package(GTest)
if(GTest_FOUND)
  include(GoogleTest)

  add_executable(buffer_test src/frt/buffer_test.cpp)
  target_link_libraries(buffer_test frt GTest::gtest_main)
  gtest_discover_tests(buffer_test)
endif()

add_subdirectory(tests/xdma)
