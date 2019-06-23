find_path(XCLBIN_INCLUDE_DIR NAMES xclbin.h PATHS "$ENV{XILINX_XRT}/include")
find_library(XRT_CORE_LIBRARY NAMES xrt_core PATHS "$ENV{XILINX_XRT}/lib")
find_library(XILINXOPENCL_LIBRARY
             NAMES xilinxopencl
             PATHS "$ENV{XILINX_XRT}/lib")

find_package_handle_standard_args(XRT
                                  FOUND_VAR
                                  XRT_FOUND
                                  REQUIRED_VARS
                                  XRT_CORE_LIBRARY
                                  XILINXOPENCL_LIBRARY
                                  XCLBIN_INCLUDE_DIR)

mark_as_advanced(XCLBIN_INCLUDE_DIR XRT_LIBRARY)

if(XRT_FOUND AND NOT TARGET xrt::xrt)
  add_library(xrt::xrt IMPORTED INTERFACE)
  set_target_properties(xrt::xrt
                        PROPERTIES INTERFACE_LINK_LIBRARIES "${XRT_LIBRARY}")
  set_target_properties(xrt::xrt
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${XCLBIN_INCLUDE_DIRS}")
endif()
