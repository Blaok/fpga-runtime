include(../../cmake/FindSDx.cmake)

add_executable(qdma-vadd)
target_sources(qdma-vadd PRIVATE qdma-host.cpp)
target_compile_features(qdma-vadd PRIVATE cxx_auto_type)
target_link_libraries(qdma-vadd PRIVATE frt pthread)

if(NOT XRT_PLATFORM)
  set(XRT_PLATFORM xilinx_u200_qdma_201910_1)
endif()
set(KERNEL VecAdd)

add_xocc_targets(${CMAKE_CURRENT_BINARY_DIR}
                 PREFIX qdma
                 KERNEL ${KERNEL}
                 PLATFORM ${XRT_PLATFORM}
                 INPUT qdma-kernel.cpp
                 SW_EMU_XCLBIN sw_emu_xclbin
                 HW_EMU_XCLBIN hw_emu_xclbin
                 HW_XCLBIN hw_xclbin)

add_custom_target(qdma-csim
                  COMMAND qdma-vadd
                          $<TARGET_PROPERTY:${sw_emu_xclbin},FILE_NAME> 1000
                  DEPENDS qdma-vadd ${sw_emu_xclbin}
                  WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
add_custom_target(qdma-cosim
                  COMMAND qdma-vadd
                          $<TARGET_PROPERTY:${hw_emu_xclbin},FILE_NAME> 1000
                  DEPENDS qdma-vadd ${hw_emu_xclbin}
                  WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
add_custom_target(qdma-hw
                  COMMAND qdma-vadd $<TARGET_PROPERTY:${hw_xclbin},FILE_NAME>
                          10000000
                  DEPENDS qdma-vadd ${hw_xclbin}
                  WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
add_custom_target(qdma-emu DEPENDS qdma-csim qdma-cosim)

add_test(NAME qdma-csim
         COMMAND ${CMAKE_COMMAND}
                 --build
                 ${CMAKE_BINARY_DIR}
                 --target
                 qdma-csim)
add_test(NAME qdma-cosim
         COMMAND ${CMAKE_COMMAND}
                 --build
                 ${CMAKE_BINARY_DIR}
                 --target
                 qdma-cosim)
