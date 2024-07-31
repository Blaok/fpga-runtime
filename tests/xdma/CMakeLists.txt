include(../../cmake/FindSDx.cmake)

add_executable(xdma-vadd)
target_sources(xdma-vadd
               PRIVATE
               xdma-host.cpp
               xdma-kernel.cpp)
target_compile_features(xdma-vadd PRIVATE cxx_auto_type)
target_link_libraries(xdma-vadd PRIVATE frt)

if(NOT XRT_PLATFORM)
  set(XRT_PLATFORM xilinx_u250_xdma_201830_2)
endif()
set(KERNEL VecAdd)

if (${XRT_PLATFORM} MATCHES "_(u50|u280)_")
  set(DRAM HBM)
else()
  set(DRAM DDR)
endif()
add_xocc_targets(${CMAKE_CURRENT_BINARY_DIR}
                 PREFIX xdma
                 KERNEL ${KERNEL}
                 PLATFORM ${XRT_PLATFORM}
                 INPUT xdma-kernel.cpp
                 DRAM_MAPPING gmem0:${DRAM}[0] gmem1:${DRAM}[1] gmem2:${DRAM}[2]
                 SW_EMU_XCLBIN sw_emu_xclbin
                 HW_EMU_XCLBIN hw_emu_xclbin
                 HW_XCLBIN hw_xclbin)

file(
  DOWNLOAD
  https://github.com/Licheng-Guo/tapa-fast-cosim/raw/02ecd87c4d24d6513d8c0d15af8046d5ee05a2ae/example/vadd/vadd.xo
  ${CMAKE_CURRENT_BINARY_DIR}/vadd.xo
  TLS_VERIFY ON
)

add_custom_target(xdma-csim
                  COMMAND xdma-vadd
                          $<TARGET_PROPERTY:${sw_emu_xclbin},FILE_NAME> 1000000
                  DEPENDS xdma-vadd ${sw_emu_xclbin}
                  WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
add_custom_target(xdma-xosim
                  COMMAND xdma-vadd ${CMAKE_CURRENT_BINARY_DIR}/vadd.xo 1000
                  DEPENDS xdma-vadd ${CMAKE_CURRENT_BINARY_DIR}/vadd.xo
                  WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
add_custom_target(xdma-cosim
                  COMMAND xdma-vadd
                          $<TARGET_PROPERTY:${hw_emu_xclbin},FILE_NAME> 1000
                  DEPENDS xdma-vadd ${hw_emu_xclbin}
                  WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
add_custom_target(xdma-hw
                  COMMAND xdma-vadd $<TARGET_PROPERTY:${hw_xclbin},FILE_NAME>
                          10000000
                  DEPENDS xdma-vadd ${hw_xclbin}
                  WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
add_custom_target(xdma-emu DEPENDS xdma-csim xdma-xosim xdma-cosim)

add_test(NAME xdma-csim
         COMMAND ${CMAKE_COMMAND}
                 --build
                 ${CMAKE_BINARY_DIR}
                 --target
                 xdma-csim)
add_test(NAME xdma-cosim
         COMMAND ${CMAKE_COMMAND}
                 --build
                 ${CMAKE_BINARY_DIR}
                 --target
                 xdma-cosim)
