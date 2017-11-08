set(SENSORY_HOME "${ANKI_EXTERNAL_DIR}/anki-thirdparty/sensory")
set(SENSORY_INCLUDE_PATH "${SENSORY_HOME}/TrulyHandsfreeSDK/4.4.23/android/include")

if (ANDROID)
    set(SENSORY_LIB_PATH "${SENSORY_HOME}/TrulyHandsfreeSDK/4.4.23_noexpire/android/lib")
elseif (MACOSX)
    set(SENSORY_LIB_PATH "${SENSORY_HOME}/TrulyHandsfreeSDK/4.4.23_noexpire/x86_64-darwin/lib")
endif()

set(SENSORY_LIBS
  thf
)

set(PLATFORM_TAG "")
if (ANDROID)
    set(PLATFORM_TAG "_armeabi-v7a")
endif()

foreach(LIB ${SENSORY_LIBS})
  add_library(${LIB} STATIC IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${SENSORY_LIB_PATH}/lib${LIB}${PLATFORM_TAG}.a"
    INTERFACE_INCLUDE_DIRECTORIES
    "${SENSORY_INCLUDE_PATH}")
endforeach()
