set(FLATBUFFERS_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/flatbuffers/include")

set(FLATBUFFERS_LIBS
  flatbuffers
)

if (VICOS)
  set(FLATBUFFERS_LIB_PATH "${CORETECH_EXTERNAL_DIR}/flatbuffers/vicos")
elseif (MACOSX)
  set(FLATBUFFERS_LIB_PATH "${CORETECH_EXTERNAL_DIR}/flatbuffers/ios/Release")
endif()

foreach(LIB ${FLATBUFFERS_LIBS})
  add_library(${LIB} STATIC IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${FLATBUFFERS_LIB_PATH}/lib${LIB}.a"
    INTERFACE_INCLUDE_DIRECTORIES
    "${FLATBUFFERS_INCLUDE_PATH}")
  anki_build_target_license(${LIB} "Apache-2.0")
endforeach()
