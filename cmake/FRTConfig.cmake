find_package(OpenCL REQUIRED)
find_package(TinyXML REQUIRED PATHS ${CMAKE_CURRENT_LIST_DIR})
find_package(XRT REQUIRED PATHS ${CMAKE_CURRENT_LIST_DIR})

include("${CMAKE_CURRENT_LIST_DIR}/FRTTargets.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/SDxConfig.cmake")
