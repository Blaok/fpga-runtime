cmake_minimum_required(VERSION 3.1)

include(../xocc.cmake)

project(vadd)

add_subdirectory(frt)

add_executable(vadd test-fpga-kernel.cpp test-fpga-runtime.cpp)
target_compile_features(vadd PUBLIC cxx_auto_type)
target_link_libraries(vadd frt)

if(NOT XRT_PLATFORM)
  set(XRT_PLATFORM xilinx_u200_xdma_201830_1)
endif()
set(KERNEL VecAdd)

add_xocc_targets_with_alias(${CMAKE_CURRENT_BINARY_DIR}
                            xocc
                            ${KERNEL}
                            ${XRT_PLATFORM}
                            test-fpga-kernel.cpp
                            "gmem0:DDR[0];gmem1:DDR[1];gmem2:DDR[2]")

add_custom_target(emu DEPENDS csim cosim)
add_custom_target(
  csim
  COMMAND vadd
          $<TARGET_PROPERTY:sw_emu_xclbin.${KERNEL}.${XRT_PLATFORM},FILE_NAME>
          1000000
  DEPENDS vadd sw_emu_xclbin.${KERNEL}.${XRT_PLATFORM}
  WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
add_custom_target(
  cosim
  COMMAND vadd
          $<TARGET_PROPERTY:hw_emu_xclbin.${KERNEL}.${XRT_PLATFORM},FILE_NAME>
          1000
  DEPENDS vadd hw_emu_xclbin.${KERNEL}.${XRT_PLATFORM}
  WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
add_custom_target(
  hw
  COMMAND vadd $<TARGET_PROPERTY:hw_xclbin.${KERNEL}.${XRT_PLATFORM},FILE_NAME>
          10000000
  DEPENDS vadd hw_xclbin.${KERNEL}.${XRT_PLATFORM}
  WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
