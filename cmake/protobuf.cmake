
if(PROTOBUF_INCLUDED)
  return()
endif(PROTOBUF_INCLUDED)
set(PROTOBUF_INCLUDED true)

set(PROTOBUF_LIBS
  protobuf
)

if (VICOS)
  set(PROTOBUF_LIB_PATH "${ANKI_EXTERNAL_DIR}/protobuf/vicos/lib")
  set(PROTOBUF_INCLUDE_PATH "${ANKI_EXTERNAL_DIR}/protobuf/vicos/include")
elseif (MACOSX)
  set(PROTOBUF_LIB_PATH "${ANKI_EXTERNAL_DIR}/protobuf/mac/lib")
  set(PROTOBUF_INCLUDE_PATH "${ANKI_EXTERNAL_DIR}/protobuf/mac/include")
endif()

foreach(LIB ${PROTOBUF_LIBS})
  add_library(${LIB} STATIC IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${PROTOBUF_LIB_PATH}/lib${LIB}.a"
    INTERFACE_INCLUDE_DIRECTORIES
    "${PROTOBUF_INCLUDE_PATH}")
  anki_build_target_license(${LIB} "BSD-3,${CMAKE_SOURCE_DIR}/licenses/protobuf.license")
endforeach()
