cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)

project(frt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(OpenCL REQUIRED)
find_package(TinyXML REQUIRED)
find_package(XRT REQUIRED)

add_library(frt_static STATIC)
add_library(frt ALIAS frt_static)
add_library(frt_shared SHARED)
target_sources(frt_static
               PRIVATE
               src/frt.cpp
               PUBLIC
               src/frt.h
               src/opencl-errors.h)
target_sources(frt_shared
               PRIVATE
               src/frt.cpp
               PUBLIC
               src/frt.h
               src/opencl-errors.h)
target_compile_features(frt_static
                        PUBLIC
                        cxx_alias_templates
                        cxx_nullptr
                        cxx_variadic_templates)
target_compile_features(frt_shared
                        PUBLIC
                        cxx_alias_templates
                        cxx_nullptr
                        cxx_variadic_templates)
set_target_properties(frt_static frt_shared
                      PROPERTIES OUTPUT_NAME
                                 frt
                                 PUBLIC_HEADER
                                 "src/frt.h;src/opencl-errors.h")
target_include_directories(frt_static PUBLIC src)
target_include_directories(frt_shared PUBLIC src)
target_link_libraries(frt_static
                      PRIVATE TinyXML::TinyXML
                      PUBLIC OpenCL::OpenCL xrt::xrt)
target_link_libraries(frt_shared
                      PRIVATE TinyXML::TinyXML
                      PUBLIC OpenCL::OpenCL xrt::xrt)

include(GNUInstallDirs)
install(TARGETS frt_static frt_shared
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(CPACK_PACKAGE_NAME libfrt-dev)
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 20190622.1)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "FPGA Runtime for PCIe-Based FPGAs")

set(CPACK_DEBIAN_PACKAGE_DEPENDS "jq, libtinyxml-dev")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Blaok Chi")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

set(CPACK_GENERATOR TGZ DEB)
include(CPack)
