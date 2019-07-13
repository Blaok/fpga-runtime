include(../cmake/FindSDx.cmake)

add_executable(vadd)
target_sources(vadd
               PRIVATE
               test-frt-host.cpp
               test-frt-kernel.cpp)
target_compile_features(vadd PRIVATE cxx_auto_type)
target_link_libraries(vadd PRIVATE frt)

if(NOT XRT_PLATFORM)
  set(XRT_PLATFORM xilinx_u200_xdma_201830_1)
endif()
set(KERNEL VecAdd)

add_xocc_targets_with_alias(${CMAKE_CURRENT_BINARY_DIR}
                            ${KERNEL}
                            ${XRT_PLATFORM}
                            test-frt-kernel.cpp
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

add_test(NAME csim COMMAND make csim)
add_test(NAME cosim COMMAND make cosim)
