cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)

project(frt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
find_package(OpenCL REQUIRED)
find_package(TinyXML REQUIRED)
find_package(XRT REQUIRED)

set(frt_sources
    src/frt.cpp
    src/frt/arg_info.cpp
    src/frt/intel_opencl_device.cpp
    src/frt/opencl_device.cpp
    src/frt/xilinx_opencl_device.cpp
    src/frt/xilinx_opencl_stream.cpp
)
set(frt_compile_features
    cxx_std_14
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
)
set(frt_private_link_libraries TinyXML::TinyXML)
set(frt_public_link_libraries OpenCL::OpenCL Threads::Threads)

add_library(frt_static STATIC)
add_library(frt ALIAS frt_static)
add_library(frt_shared SHARED)
target_sources(frt_static PRIVATE ${frt_sources})
target_sources(frt_shared PRIVATE ${frt_sources})
target_compile_features(frt_static PUBLIC ${frt_compile_features})
target_compile_features(frt_shared PUBLIC ${frt_compile_features})
target_compile_definitions(frt_static PUBLIC ${frt_compile_definitions})
target_compile_definitions(frt_shared PUBLIC ${frt_compile_definitions})
set_target_properties(frt_static frt_shared PROPERTIES OUTPUT_NAME frt)
set_target_properties(frt_shared PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(frt_static PROPERTIES EXPORT_NAME frt)
target_include_directories(
  frt_static
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src> ${XRT_INCLUDE_DIR}
)
target_include_directories(
  frt_shared
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src> ${XRT_INCLUDE_DIR}
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

include(GNUInstallDirs)
install(
  TARGETS frt_static frt_shared
  EXPORT FRTTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(
  DIRECTORY "${CMAKE_SOURCE_DIR}/src/"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING
  PATTERN "*.h"
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
  FILES cmake/FindTinyXML.cmake
  RENAME TinyXMLConfig.cmake
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
set(CPACK_PACKAGE_VERSION_PATCH 20220109.2)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "FPGA Runtime for PCIe-Based FPGAs")

string(
  JOIN
  ", "
  CPACK_DEBIAN_PACKAGE_DEPENDS
  "jq"
  "libtinyxml-dev"
  "ocl-icd-opencl-dev"
  "opencl-headers"
  "tar"
  "zstd"
)
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "csh, xrt")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Blaok Chi")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

string(
  JOIN
  ", "
  CPACK_RPM_PACKAGE_REQUIRES
  "jq"
  "tinyxml-devel"
  "ocl-icd-devel"
  "opencl-headers"
  "tar"
  "zstd"
)
set(CPACK_RPM_PACKAGE_NAME frt-devel)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

set(CPACK_GENERATOR
    TGZ DEB
    CACHE STRING "Enable selected CPack generators"
)
include(CPack)

enable_testing()
add_subdirectory(tests/hbm)
add_subdirectory(tests/xdma)
