cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)

project(frt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(OpenCL REQUIRED)
find_package(TinyXML REQUIRED)

add_library(frt STATIC)
target_sources(frt
               PRIVATE
               src/fpga-runtime.cpp
               PUBLIC
               src/fpga-runtime.h
               src/opencl-errors.h)
target_compile_features(frt
                        PUBLIC
                        cxx_alias_templates
                        cxx_nullptr
                        cxx_variadic_templates)
target_include_directories(frt PUBLIC src $ENV{XILINX_XRT}/include)
target_link_libraries(frt PRIVATE TinyXML::TinyXML PUBLIC OpenCL::OpenCL)

include(GNUInstallDirs)
install(TARGETS frt
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
