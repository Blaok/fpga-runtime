if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

find_program(XOCC xocc PATHS "$ENV{XILINX_SDX}/bin")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDx
                                  FOUND_VAR
                                  SDx_FOUND
                                  REQUIRED_VARS
                                  XOCC)

function(add_xocc_compile_target
         target_name
         output
         target
         report_dir
         log_dir
         temp_dir
         input)

  get_target_property(kernel ${input} KERNEL)
  get_target_property(platform ${input} PLATFORM)
  get_target_property(dram_mapping ${input} DRAM_MAPPING)
  get_target_property(input_file ${input} FILE_NAME)
  get_filename_component(output ${output} ABSOLUTE)
  get_filename_component(input_file ${input_file} ABSOLUTE)
  if(CMAKE_BUILD_TYPE MATCHES ^$|Debug)
    set(debug_flags --debug --save-temps)
  endif()

  set(cwd /tmp/cmake.xocc.compile.$ENV{USER})
  file(MAKE_DIRECTORY ${cwd})

  add_custom_command(OUTPUT ${output}
                     COMMAND ${XOCC}
                             --output ${output}
                             --compile
                             --kernel ${kernel}
                             --platform ${platform}
                             --target ${target}
                             --report_level 2
                             --report_dir ${report_dir}
                             --log_dir ${log_dir}
                             --temp_dir ${temp_dir}
                             --xp prop:kernel.${kernel}.kernel_flags=-std=c++11
                                  ${debug_flags} ${input_file}
                     DEPENDS ${input}
                     WORKING_DIRECTORY ${cwd}
                     VERBATIM)

  add_custom_target(${target_name} DEPENDS ${output})
  set_target_properties(${target_name}
                        PROPERTIES KERNEL
                                   ${kernel}
                                   PLATFORM
                                   ${platform}
                                   TARGET
                                   ${target}
                                   FILE_NAME
                                   ${output}
                                   DRAM_MAPPING
                                   "${dram_mapping}")
endfunction()

function(add_xocc_link_target
         target_name
         output
         target
         optimize
         report_dir
         log_dir
         temp_dir
         input)

  get_target_property(kernel ${input} KERNEL)
  get_target_property(platform ${input} PLATFORM)
  get_target_property(dram_mapping ${input} DRAM_MAPPING)
  get_target_property(input_file ${input} FILE_NAME)
  get_filename_component(output ${output} ABSOLUTE)
  get_filename_component(input_file ${input_file} ABSOLUTE)
  foreach(map ${dram_mapping})
    list(APPEND dram_mapping_cflags --sp ${kernel}.m_axi_${map})
  endforeach()
  if(CMAKE_BUILD_TYPE MATCHES ^$|Debug)
    set(debug_flags --debug --save-temps)
  endif()

  set(cwd /tmp/cmake.xocc.link.$ENV{USER})
  file(MAKE_DIRECTORY ${cwd})

  add_custom_command(OUTPUT ${output}
                     COMMAND ${XOCC}
                             --output ${output}
                             --link
                             --platform ${platform}
                             --target ${target}
                             --optimize ${optimize}
                             --report_level 2
                             --report_dir ${report_dir}
                             --log_dir ${log_dir}
                             --temp_dir ${temp_dir}
                             --nk ${kernel}:1:${kernel}
                             --max_memory_ports ${kernel}
                                                ${dram_mapping_cflags}
                                                ${debug_flags}
                                                ${input_file}
                     DEPENDS ${input}
                     WORKING_DIRECTORY ${cwd}
                     VERBATIM)

  add_custom_target(${target_name} DEPENDS ${output})
  set_target_properties(${target_name}
                        PROPERTIES KERNEL
                                   ${kernel}
                                   PLATFORM
                                   ${platform}
                                   TARGET
                                   ${target}
                                   FILE_NAME
                                   ${output}
                                   DRAM_MAPPING
                                   "${dram_mapping}")

endfunction()

function(add_xocc_targets
         output_dir
         kernel
         platform
         input_file
         dram_mapping)
  set(hls_src_target hls_src.${kernel}.${platform})
  set(sw_emu_xo_target sw_emu_xo.${kernel}.${platform})
  set(hw_xo_target hw_xo.${kernel}.${platform})
  set(sw_emu_xclbin_target sw_emu_xclbin.${kernel}.${platform})
  set(hw_emu_xclbin_target hw_emu_xclbin.${kernel}.${platform})
  set(hw_xclbin_target hw_xclbin.${kernel}.${platform})
  add_custom_target(${hls_src_target} DEPENDS ${input_file})
  set_target_properties(${hls_src_target}
                        PROPERTIES KERNEL
                                   ${kernel}
                                   PLATFORM
                                   ${platform}
                                   FILE_NAME
                                   ${input_file}
                                   DRAM_MAPPING
                                   "${dram_mapping}")

  if(CMAKE_BUILD_TYPE MATCHES ^$|Debug)
    set(optimize quick)
  elseif(CMAKE_BUILD_TYPE MATCHES Release)
    set(optimize 3)
  else()
    set(optimize 0)
  endif()

  set(sw_emu_xo ${kernel}.${platform}.sw_emu.xo)
  set(hw_xo ${kernel}.${platform}.hw.xo)
  set(sw_emu_xclbin ${kernel}.${platform}.sw_emu.xclbin)
  set(hw_emu_xclbin ${kernel}.${platform}.hw_emu.xclbin)
  set(hw_xclbin ${kernel}.${platform}.hw.xclbin)
  add_xocc_compile_target(${sw_emu_xo_target}
                          ${output_dir}/${sw_emu_xo}
                          sw_emu
                          ${output_dir}/${sw_emu_xo}.report
                          ${output_dir}/${sw_emu_xo}.log
                          ${output_dir}/${sw_emu_xo}.temp
                          ${hls_src_target})
  add_xocc_compile_target(${hw_xo_target}
                          ${output_dir}/${hw_xo}
                          hw
                          ${output_dir}/${hw_xo}.report
                          ${output_dir}/${hw_xo}.log
                          ${output_dir}/${hw_xo}.temp
                          ${hls_src_target})
  add_xocc_link_target(${sw_emu_xclbin_target}
                       ${output_dir}/${sw_emu_xclbin}
                       sw_emu
                       ${optimize}
                       ${output_dir}/${sw_emu_xclbin}.report
                       ${output_dir}/${sw_emu_xclbin}.log
                       ${output_dir}/${sw_emu_xclbin}.temp
                       ${sw_emu_xo_target})
  add_xocc_link_target(${hw_emu_xclbin_target}
                       ${output_dir}/${hw_emu_xclbin}
                       hw_emu
                       ${optimize}
                       ${output_dir}/${hw_emu_xclbin}.report
                       ${output_dir}/${hw_emu_xclbin}.log
                       ${output_dir}/${hw_emu_xclbin}.temp
                       ${hw_xo_target})
  add_xocc_link_target(${hw_xclbin_target}
                       ${output_dir}/${hw_xclbin}
                       hw
                       ${optimize}
                       ${output_dir}/${hw_xclbin}.report
                       ${output_dir}/${hw_xclbin}.log
                       ${output_dir}/${hw_xclbin}.temp
                       ${hw_xo_target})
endfunction()

function(add_xocc_targets_with_alias
         output_dir
         kernel
         platform
         input_file
         dram_mapping)
  add_xocc_targets(${output_dir}
                   ${kernel}
                   ${platform}
                   ${input_file}
                   "${dram_mapping}")
  set(hw_xo_target hw_xo.${kernel}.${platform})
  set(sw_emu_xclbin_target sw_emu_xclbin.${kernel}.${platform})
  set(hw_emu_xclbin_target hw_emu_xclbin.${kernel}.${platform})
  set(hw_xclbin_target hw_xclbin.${kernel}.${platform})
  add_custom_target(hls DEPENDS ${hw_xo_target})
  add_custom_target(sw_emu_xclbin DEPENDS ${sw_emu_xclbin_target})
  add_custom_target(hw_emu_xclbin DEPENDS ${hw_emu_xclbin_target})
  add_custom_target(bitstream DEPENDS ${hw_xclbin_target})
  add_custom_target(xclbins DEPENDS sw_emu_xclbin hw_emu_xclbin bitstream)
endfunction()
