include(../../cmake/FindSDx.cmake)

add_executable(hbm-vadd)
target_sources(hbm-vadd PRIVATE ../xdma/xdma-host.cpp ../xdma/xdma-kernel.cpp)
target_compile_features(hbm-vadd PRIVATE cxx_auto_type)
target_link_libraries(hbm-vadd PRIVATE frt)

if(NOT XRT_PLATFORM)
  set(XRT_PLATFORM xilinx_u50_xdma_201920_1)
endif()
set(KERNEL VecAdd)

add_xocc_targets(
  ${CMAKE_CURRENT_BINARY_DIR}
  PREFIX hbm
  KERNEL ${KERNEL}
  PLATFORM ${XRT_PLATFORM}
  INPUT ../xdma/xdma-kernel.cpp
  DRAM_MAPPING gmem0:HBM[0] gmem1:HBM[1] gmem2:HBM[2]
  SW_EMU_XCLBIN
  sw_emu_xclbin
  HW_EMU_XCLBIN
  hw_emu_xclbin
  HW_XCLBIN
  hw_xclbin)

add_custom_target(
  hbm-csim
  COMMAND hbm-vadd $<TARGET_PROPERTY:${sw_emu_xclbin},FILE_NAME> 1000000
  DEPENDS hbm-vadd ${sw_emu_xclbin}
  WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
add_custom_target(
  hbm-cosim
  COMMAND hbm-vadd $<TARGET_PROPERTY:${hw_emu_xclbin},FILE_NAME> 1000
  DEPENDS hbm-vadd ${hw_emu_xclbin}
  WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
add_custom_target(
  hbm-hw
  COMMAND hbm-vadd $<TARGET_PROPERTY:${hw_xclbin},FILE_NAME> 10000000
  DEPENDS hbm-vadd ${hw_xclbin}
  WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
add_custom_target(hbm-emu DEPENDS hbm-csim hbm-cosim)

add_test(NAME hbm-csim COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
                               --target hbm-csim)
add_test(NAME hbm-cosim COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
                                --target hbm-cosim)
