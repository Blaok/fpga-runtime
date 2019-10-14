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

function(add_xocc_compile_target target_name)
  # Generates a `.xo` file via `xocc --compile`.
  #
  # The added target will have the following properties:
  #
  # * KERNEL
  # * PLATFORM
  # * TARGET
  # * FILE_NAME
  # * DRAM_MAPPING
  #
  # Positional Arguments:
  #
  # * target_name: Name of the added cmake target.
  #
  # Named Arguments:
  #
  # * TARGET: Synthesis target (sw_emu|hw_emu|hw) [--target].
  # * OUTPUT: Output filename [--output].
  # * REPORT_DIR: A directory to copy report files to [--report_dir].
  # * LOG_DIR: A directory to copy internally generated log files to
  #   [--log_dir].
  # * TEMP_DIR: A directory to use for temporary files and directories
  #   [--temp_dir].
  # * INPUT: Input HLS source target.
  # * SAVE_TEMPS: Boolean, whether to keep intermediate files.
  #
  # Other Arguments:
  #
  # * Optional, will be passed to `xocc --compile` directly.

  # parse arguments and extract information
  cmake_parse_arguments(XOCC_COMPILE
                        "SAVE_TEMPS"
                        "TARGET;OUTPUT;REPORT_DIR;LOG_DIR;TEMP_DIR;INPUT"
                        ""
                        ${ARGN})
  set(target ${XOCC_COMPILE_TARGET})
  set(output ${XOCC_COMPILE_OUTPUT})
  set(report_dir ${XOCC_COMPILE_REPORT_DIR})
  set(log_dir ${XOCC_COMPILE_LOG_DIR})
  set(temp_dir ${XOCC_COMPILE_TEMP_DIR})
  set(input ${XOCC_COMPILE_INPUT})
  get_target_property(kernel ${input} KERNEL)
  get_target_property(platform ${input} PLATFORM)
  get_target_property(dram_mapping ${input} DRAM_MAPPING)
  get_target_property(input_file ${input} FILE_NAME)
  get_filename_component(output ${output} ABSOLUTE)
  get_filename_component(input_file ${input_file} ABSOLUTE)

  # compose the xocc compile command
  set(xocc_cmd ${XOCC} --compile)
  list(APPEND xocc_cmd --output ${output})
  list(APPEND xocc_cmd --kernel ${kernel})
  list(APPEND xocc_cmd --platform ${platform})
  list(APPEND xocc_cmd --target ${target})
  list(APPEND xocc_cmd --report_level 2)
  list(APPEND xocc_cmd --report_dir ${report_dir})
  list(APPEND xocc_cmd --log_dir ${log_dir})
  list(APPEND xocc_cmd --temp_dir ${temp_dir})
  list(APPEND xocc_cmd --xp prop:kernel.${kernel}.kernel_flags=-std=c++11)
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    list(APPEND xocc_cmd --debug)
  endif()
  if(${XOCC_COMPILE_SAVE_TEMPS})
    list(APPEND xocc_cmd --save-temps)
  endif()
  list(APPEND xocc_cmd ${input_file})
  list(APPEND xocc_cmd ${XOCC_COMPILE_UNPARSED_ARGUMENTS})
  list(APPEND xocc_cmd "&&;find;${output};-newer;${input_file}")
  list(APPEND xocc_cmd "|;grep;${output};-q")

  set(cwd /tmp/cmake.xocc.compile.$ENV{USER})
  file(MAKE_DIRECTORY ${cwd})

  add_custom_command(OUTPUT ${output}
                     COMMAND ${xocc_cmd}
                     DEPENDS ${input} ${input_file}
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

function(add_xocc_link_target target_name)
  # Generates a `.xclbin` file via `xocc --link`.
  #
  # The added target will have the following properties:
  #
  # * KERNEL
  # * PLATFORM
  # * TARGET
  # * FILE_NAME
  # * DRAM_MAPPING
  #
  # Positional Arguments:
  #
  # * target_name: Name of the added cmake target.
  #
  # Named Arguments:
  #
  # * TARGET: Synthesis target (sw_emu|hw_emu|hw) [--target].
  # * OUTPUT: Output filename [--output].
  # * OPTIMIZE: Optimize level [--optimize].
  # * REPORT_DIR: A directory to copy report files to [--report_dir].
  # * LOG_DIR: A directory to copy internally generated log files to
  #   [--log_dir].
  # * TEMP_DIR: A directory to use for temporary files and directories
  #   [--temp_dir].
  # * INPUT: Input target generated via add_xocc_compile_target.
  # * SAVE_TEMPS: Boolean, whether to keep intermediate files.
  #
  # Other Arguments:
  #
  # * Optional, will be passed to `xocc --link` directly.

  # parse arguments and extract information
  cmake_parse_arguments(
    XOCC_LINK
    "SAVE_TEMPS"
    "TARGET;OUTPUT;OPTIMIZE;REPORT_DIR;LOG_DIR;TEMP_DIR;INPUT"
    ""
    ${ARGN})
  set(target ${XOCC_LINK_TARGET})
  set(output ${XOCC_LINK_OUTPUT})
  set(optimize ${XOCC_LINK_OPTIMIZE})
  set(report_dir ${XOCC_LINK_REPORT_DIR})
  set(log_dir ${XOCC_LINK_LOG_DIR})
  set(temp_dir ${XOCC_LINK_TEMP_DIR})
  set(input ${XOCC_LINK_INPUT})
  get_target_property(kernel ${input} KERNEL)
  get_target_property(platform ${input} PLATFORM)
  get_target_property(dram_mapping ${input} DRAM_MAPPING)
  get_target_property(input_file ${input} FILE_NAME)
  get_filename_component(output ${output} ABSOLUTE)
  get_filename_component(input_file ${input_file} ABSOLUTE)

  # compose the xocc link command
  set(xocc_cmd ${XOCC} --link)
  list(APPEND xocc_cmd --output ${output})
  list(APPEND xocc_cmd --kernel ${kernel})
  list(APPEND xocc_cmd --platform ${platform})
  list(APPEND xocc_cmd --target ${target})
  list(APPEND xocc_cmd --report_level 2)
  list(APPEND xocc_cmd --report_dir ${report_dir})
  list(APPEND xocc_cmd --log_dir ${log_dir})
  list(APPEND xocc_cmd --temp_dir ${temp_dir})
  list(APPEND xocc_cmd --optimize ${optimize})
  list(APPEND xocc_cmd --nk ${kernel}:1:${kernel})
  list(APPEND xocc_cmd --max_memory_ports ${kernel})
  foreach(map ${dram_mapping})
    list(APPEND xocc_cmd --sp ${kernel}.m_axi_${map})
  endforeach()
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    list(APPEND xocc_cmd --debug)
  endif()
  if(${XOCC_LINK_SAVE_TEMPS})
    list(APPEND xocc_cmd --save-temps)
  endif()
  list(APPEND xocc_cmd ${input_file})
  list(APPEND xocc_cmd ${XOCC_LINK_UNPARSED_ARGUMENTS})
  list(APPEND xocc_cmd "&&;find;${output};-newer;${input_file}")
  list(APPEND xocc_cmd "|;grep;${output};-q")

  set(cwd /tmp/cmake.xocc.link.$ENV{USER})
  file(MAKE_DIRECTORY ${cwd})

  add_custom_command(OUTPUT ${output}
                     COMMAND ${xocc_cmd}
                     DEPENDS ${input} ${input_file}
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

function(add_xocc_hw_link_targets output_dir)
  # Add cmake targets for hardware simulation and hardware execution.
  #
  # Positional Arguments:
  #
  # * output_dir: Output directory.
  #
  # Required Named Arguments:
  #
  # * KERNEL: Kernel name [--kernel].
  # * PLATFORM: Platform [--platform].
  # * INPUT: Input filename.
  #
  # Optional Named Arguments:
  #
  # * PREFIX: Prefix of the generated targets.
  # * DRAM_MAPPING: A list of mappings from variable name to DDR banks (e.g.
  #   gmem0:DDR[0]).
  # * HW_XO: Returns the name of the hw_xo_target.
  # * HW_EMU_XCLBIN: Returns the name of the hw_emu_xclbin_target.
  # * HW_XCLBIN: Returns the name of the hw_xclbin_target.

  set(one_value_keywords KERNEL PLATFORM INPUT)
  list(APPEND one_value_keywords
              PREFIX
              HW_XO
              HW_EMU_XCLBIN
              HW_XCLBIN)
  cmake_parse_arguments(XOCC
                        ""
                        "${one_value_keywords}"
                        "DRAM_MAPPING"
                        ${ARGN})
  if(XOCC_PREFIX)
    set(prefix ${XOCC_PREFIX}.)
  endif()
  if(TARGET ${XOCC_INPUT})
    get_target_property(kernel ${XOCC_INPUT} KERNEL)
    get_target_property(platform ${XOCC_INPUT} PLATFORM)
    get_target_property(input_file ${XOCC_INPUT} FILE_NAME)
    get_target_property(dram_mapping ${XOCC_INPUT} DRAM_MAPPING)
    set(prefix ${prefix}${kernel}.${platform})
    set(hw_xo_target ${XOCC_INPUT})
  else()
    set(kernel ${XOCC_KERNEL})
    set(platform ${XOCC_PLATFORM})
    set(input_file ${XOCC_INPUT})
    set(dram_mapping ${XOCC_DRAM_MAPPING})
    set(prefix ${prefix}${kernel}.${platform})
    set(hw_xo_target ${prefix}.hw_xo)
    add_custom_target(${hw_xo_target} DEPENDS ${input_file})
    set_target_properties(${hw_xo_target}
                          PROPERTIES KERNEL
                                     ${kernel}
                                     PLATFORM
                                     ${platform}
                                     TARGET
                                     hw
                                     FILE_NAME
                                     ${input_file}
                                     DRAM_MAPPING
                                     "${dram_mapping}")
  endif()

  set(hw_emu_xclbin_target ${prefix}.hw_emu_xclbin)
  set(hw_xclbin_target ${prefix}.hw_xclbin)
  if(XOCC_HW_XO)
    set(${XOCC_HW_XO} ${hw_xo_target} PARENT_SCOPE)
  endif()
  if(XOCC_HW_EMU_XCLBIN)
    set(${XOCC_HW_EMU_XCLBIN} ${hw_emu_xclbin_target} PARENT_SCOPE)
  endif()
  if(XOCC_HW_XCLBIN)
    set(${XOCC_HW_XCLBIN} ${hw_xclbin_target} PARENT_SCOPE)
  endif()

  if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(optimize quick)
  elseif(CMAKE_BUILD_TYPE MATCHES Release)
    set(optimize 3)
  else()
    set(optimize 0)
  endif()

  set(hw_emu_xclbin ${prefix}.hw_emu.xclbin)
  set(hw_xclbin ${prefix}.hw.xclbin)
  add_xocc_link_target(${hw_emu_xclbin_target}
                       OUTPUT ${output_dir}/${hw_emu_xclbin}
                       TARGET hw_emu
                       OPTIMIZE ${optimize}
                       REPORT_DIR ${output_dir}/${hw_emu_xclbin}.report
                       LOG_DIR ${output_dir}/${hw_emu_xclbin}.log
                       TEMP_DIR ${output_dir}/${hw_emu_xclbin}.temp
                       INPUT ${hw_xo_target}
                       SAVE_TEMPS)
  add_xocc_link_target(${hw_xclbin_target}
                       OUTPUT ${output_dir}/${hw_xclbin}
                       TARGET hw
                       OPTIMIZE ${optimize}
                       REPORT_DIR ${output_dir}/${hw_xclbin}.report
                       LOG_DIR ${output_dir}/${hw_xclbin}.log
                       TEMP_DIR ${output_dir}/${hw_xclbin}.temp
                       INPUT ${hw_xo_target}
                       SAVE_TEMPS)
endfunction()

function(add_xocc_targets output_dir)
  # Add cmake targets for software / hardware simulation and hardware execution.
  #
  # Positional Arguments:
  #
  # * output_dir: Output directory.
  #
  # Required Named Arguments:
  #
  # * KERNEL: Kernel name [--kernel].
  # * PLATFORM: Platform [--platform].
  # * INPUT: Input filename.
  #
  # Optional Named Arguments:
  #
  # * PREFIX: Prefix of the generated targets.
  # * DRAM_MAPPING: A list of mappings from variable name to DDR banks (e.g.
  #   gmem0:DDR[0]).
  # * HLS_SRC: Returns the name of the hls_src_target.
  # * SW_EMU_XO: Returns the name of the sw_emu_xo_target.
  # * HW_XO: Returns the name of the hw_xo_target.
  # * SW_EMU_XCLBIN: Returns the name of the sw_emu_xclbin_target.
  # * HW_EMU_XCLBIN: Returns the name of the hw_emu_xclbin_target.
  # * HW_XCLBIN: Returns the name of the hw_xclbin_target.

  set(one_value_keywords KERNEL PLATFORM INPUT)
  list(APPEND one_value_keywords
              PREFIX
              HLS_SRC
              SW_EMU_XO
              HW_XO
              SW_EMU_XCLBIN
              HW_EMU_XCLBIN
              HW_XCLBIN)
  cmake_parse_arguments(XOCC
                        ""
                        "${one_value_keywords}"
                        "DRAM_MAPPING"
                        ${ARGN})
  set(kernel ${XOCC_KERNEL})
  set(platform ${XOCC_PLATFORM})
  set(input_file ${XOCC_INPUT})
  set(dram_mapping ${XOCC_DRAM_MAPPING})
  if(XOCC_PREFIX)
    set(prefix ${XOCC_PREFIX}.)
  endif()
  set(prefix ${prefix}${kernel}.${platform})
  set(hls_src_target ${prefix}.hls_src)
  set(sw_emu_xo_target ${prefix}.sw_emu_xo)
  set(hw_xo_target ${prefix}.hw_xo)
  set(sw_emu_xclbin_target ${prefix}.sw_emu_xclbin)
  set(hw_emu_xclbin_target ${prefix}.hw_emu_xclbin)
  set(hw_xclbin_target ${prefix}.hw_xclbin)
  if(XOCC_HLS_SRC)
    set(${XOCC_HLS_SRC} ${hls_src_target} PARENT_SCOPE)
  endif()
  if(XOCC_SW_EMU_XO)
    set(${XOCC_SW_EMU_XO} ${sw_emu_xo_target} PARENT_SCOPE)
  endif()
  if(XOCC_HW_XO)
    set(${XOCC_HW_XO} ${hw_xo_target} PARENT_SCOPE)
  endif()
  if(XOCC_SW_EMU_XCLBIN)
    set(${XOCC_SW_EMU_XCLBIN} ${sw_emu_xclbin_target} PARENT_SCOPE)
  endif()
  if(XOCC_HW_EMU_XCLBIN)
    set(${XOCC_HW_EMU_XCLBIN} ${hw_emu_xclbin_target} PARENT_SCOPE)
  endif()
  if(XOCC_HW_XCLBIN)
    set(${XOCC_HW_XCLBIN} ${hw_xclbin_target} PARENT_SCOPE)
  endif()
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

  if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(optimize quick)
  elseif(CMAKE_BUILD_TYPE MATCHES Release)
    set(optimize 3)
  else()
    set(optimize 0)
  endif()

  set(sw_emu_xo ${prefix}.sw_emu.xo)
  set(hw_xo ${prefix}.hw.xo)
  set(sw_emu_xclbin ${prefix}.sw_emu.xclbin)
  set(hw_emu_xclbin ${prefix}.hw_emu.xclbin)
  set(hw_xclbin ${prefix}.hw.xclbin)
  add_xocc_compile_target(${sw_emu_xo_target}
                          OUTPUT ${output_dir}/${sw_emu_xo}
                          TARGET sw_emu
                          REPORT_DIR ${output_dir}/${sw_emu_xo}.report
                          LOG_DIR ${output_dir}/${sw_emu_xo}.log
                          TEMP_DIR ${output_dir}/${sw_emu_xo}.temp
                          INPUT ${hls_src_target}
                          SAVE_TEMPS)
  add_xocc_compile_target(${hw_xo_target}
                          OUTPUT ${output_dir}/${hw_xo}
                          TARGET hw
                          REPORT_DIR ${output_dir}/${hw_xo}.report
                          LOG_DIR ${output_dir}/${hw_xo}.log
                          TEMP_DIR ${output_dir}/${hw_xo}.temp
                          INPUT ${hls_src_target}
                          SAVE_TEMPS)
  add_xocc_link_target(${sw_emu_xclbin_target}
                       OUTPUT ${output_dir}/${sw_emu_xclbin}
                       TARGET sw_emu
                       OPTIMIZE ${optimize}
                       REPORT_DIR ${output_dir}/${sw_emu_xclbin}.report
                       LOG_DIR ${output_dir}/${sw_emu_xclbin}.log
                       TEMP_DIR ${output_dir}/${sw_emu_xclbin}.temp
                       INPUT ${sw_emu_xo_target}
                       SAVE_TEMPS)
  add_xocc_link_target(${hw_emu_xclbin_target}
                       OUTPUT ${output_dir}/${hw_emu_xclbin}
                       TARGET hw_emu
                       OPTIMIZE ${optimize}
                       REPORT_DIR ${output_dir}/${hw_emu_xclbin}.report
                       LOG_DIR ${output_dir}/${hw_emu_xclbin}.log
                       TEMP_DIR ${output_dir}/${hw_emu_xclbin}.temp
                       INPUT ${hw_xo_target}
                       SAVE_TEMPS)
  add_xocc_link_target(${hw_xclbin_target}
                       OUTPUT ${output_dir}/${hw_xclbin}
                       TARGET hw
                       OPTIMIZE ${optimize}
                       REPORT_DIR ${output_dir}/${hw_xclbin}.report
                       LOG_DIR ${output_dir}/${hw_xclbin}.log
                       TEMP_DIR ${output_dir}/${hw_xclbin}.temp
                       INPUT ${hw_xo_target}
                       SAVE_TEMPS)
endfunction()

function(add_xocc_targets_with_alias)
  # Add cmake targets for software / hardware simulation and hardware execution,
  # with global alias hls, bitstream, sw_emu_xclbin, hw_emu_xclbin, and xclbins.
  #
  # Takes the same set of arguments as add_xocc_targets.

  cmake_parse_arguments(XOCC
                        ""
                        "HW_XO;SW_EMU_XCLBIN;HW_EMU_XCLBIN;HW_XCLBIN"
                        ""
                        ${ARGV})
  add_xocc_targets(${ARGV}
                   HW_XO hw_xo_target
                   SW_EMU_XCLBIN sw_emu_xclbin_target
                   HW_EMU_XCLBIN hw_emu_xclbin_target
                   HW_XCLBIN hw_xclbin_target)
  if(XOCC_HW_XO)
    set(${XOCC_HW_XO} ${hw_xo_target} PARENT_SCOPE)
  endif()
  if(XOCC_SW_EMU_XCLBIN)
    set(${XOCC_SW_EMU_XCLBIN} ${sw_emu_xclbin_target} PARENT_SCOPE)
  endif()
  if(XOCC_HW_EMU_XCLBIN)
    set(${XOCC_HW_EMU_XCLBIN} ${hw_emu_xclbin_target} PARENT_SCOPE)
  endif()
  if(XOCC_HW_XCLBIN)
    set(${XOCC_HW_XCLBIN} ${hw_xclbin_target} PARENT_SCOPE)
  endif()

  add_custom_target(hls DEPENDS ${hw_xo_target})
  add_custom_target(sw_emu_xclbin DEPENDS ${sw_emu_xclbin_target})
  add_custom_target(hw_emu_xclbin DEPENDS ${hw_emu_xclbin_target})
  add_custom_target(bitstream DEPENDS ${hw_xclbin_target})
  add_custom_target(xclbins DEPENDS sw_emu_xclbin hw_emu_xclbin bitstream)
endfunction()
