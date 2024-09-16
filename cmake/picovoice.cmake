set(PICOVOICE_HOME "${ANKI_THIRD_PARTY_DIR}/picovoice")
set(PICOVOICE_INCLUDE_PATH "${PICOVOICE_HOME}/vicos/include")

if (VICOS)
    set(PICOVOICE_LIB_PATH "${PICOVOICE_HOME}/vicos/lib")
endif()

# not actually porcupine. actually an interface program for a server program

set(PICOVOICE_LIBS
  pv_porcupine_interface
)

foreach(LIB ${PICOVOICE_LIBS})
  add_library(${LIB} STATIC IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${PICOVOICE_LIB_PATH}/lib${LIB}.a"
    INTERFACE_INCLUDE_DIRECTORIES
    "${PICOVOICE_INCLUDE_PATH}")
  anki_build_target_license(${LIB} "Apache-2.0,${CMAKE_SOURCE_DIR}/licenses/flatbuffers.license")
endforeach()