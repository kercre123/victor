set(SENSORY_V6_HOME "${ANKI_EXTERNAL_DIR}/anki-thirdparty/sensory")
set(SENSORY_V6_INCLUDE_PATH "${SENSORY_V6_HOME}/TrulyHandsfreeSDK/6.5.1/include")

if (VICOS)
    set(SENSORY_V6_LIB_PATH "${SENSORY_V6_HOME}/TrulyHandsfreeSDK/6.5.1/lib/arm-linux-gnueabi")
elseif (MACOSX)
    set(SENSORY_V6_LIB_PATH "${SENSORY_V6_HOME}/TrulyHandsfreeSDK/6.5.1/lib/macos")
endif()

set(SENSORY_V6_LIBS
  snsr
)

foreach(LIB ${SENSORY_V6_LIBS})
  add_library(${LIB} STATIC IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${SENSORY_V6_LIB_PATH}/lib${LIB}.a"
    INTERFACE_INCLUDE_DIRECTORIES
    "${SENSORY_V6_INCLUDE_PATH}")
#  anki_build_target_license(${LIB} "Commercial")
endforeach()
