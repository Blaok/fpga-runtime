find_path(TinyXML_INCLUDE_DIR NAMES tinyxml.h)
find_library(TinyXML_LIBRARY NAMES tinyxml)

find_package_handle_standard_args(TinyXML
                                  FOUND_VAR
                                  TinyXML_FOUND
                                  REQUIRED_VARS
                                  TinyXML_LIBRARY
                                  TinyXML_INCLUDE_DIR)

mark_as_advanced(TinyXML_INCLUDE_DIR TinyXML_LIBRARY)

if(TinyXML_FOUND AND NOT TARGET TinyXML::TinyXML)
  add_library(TinyXML::TinyXML IMPORTED INTERFACE)
  set_target_properties(
    TinyXML::TinyXML
    PROPERTIES INTERFACE_LINK_LIBRARIES "${TinyXML_LIBRARY}")
  set_target_properties(TinyXML::TinyXML
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${TinyXML_INCLUDE_DIRS}")
endif()
