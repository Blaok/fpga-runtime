find_path(
  XRT_INCLUDE_DIR
  NAMES xclbin.h
  PATHS "$ENV{XILINX_XRT}/include")
find_library(
  XILINXOPENCL_LIBRARY
  NAMES xilinxopencl
  PATHS "$ENV{XILINX_XRT}/lib")
get_filename_component(XRT_VERSION ${XILINXOPENCL_LIBRARY} REALPATH)
string(REGEX REPLACE "^.*\.so\." "" XRT_VERSION ${XRT_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  XRT
  FOUND_VAR XRT_FOUND
  VERSION_VAR XRT_VERSION
  REQUIRED_VARS XILINXOPENCL_LIBRARY XRT_INCLUDE_DIR
)

mark_as_advanced(XRT_INCLUDE_DIR XRT_LIBRARY)

if(XRT_FOUND AND NOT TARGET xrt::xrt)
  add_library(xrt::xrt IMPORTED INTERFACE)
  set_target_properties(xrt::xrt PROPERTIES INTERFACE_LINK_LIBRARIES
                                            ${XILINXOPENCL_LIBRARY})
  set_target_properties(xrt::xrt PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                            "${XRT_INCLUDE_DIR}")
endif()
