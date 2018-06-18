set(PROTOBUF_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/build/protobuf/vicos/include")

set(PROTOBUF_LIBS
  protobuf-lite
)

if (VICOS)
  set(PROTOBUF_LIB_PATH "${CORETECH_EXTERNAL_DIR}/build/protobuf/vicos/lib")
elseif (MACOSX)
  set(PROTOBUF_LIB_PATH "${CORETECH_EXTERNAL_DIR}/build/protobuf/mac/lib")
endif()

foreach(LIB ${PROTOBUF_LIBS})
  add_library(${LIB} STATIC IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${PROTOBUF_LIB_PATH}/lib${LIB}.a"
    INTERFACE_INCLUDE_DIRECTORIES
    "${PROTOBUF_INCLUDE_PATH}")
  anki_build_target_license(${LIB} "Apache-2.0,${CMAKE_SOURCE_DIR}/licenses/protobuf.license") # TODO: USE REAL LICENSE
endforeach()
