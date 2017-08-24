set(VOICE_RECOGNITION_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/sensory/TrulyHandsfreeSDK/4.4.23/ios/include")

if (ANDROID)
    set(VOICE_RECOGNITION_LIB_PATH "${CORETECH_EXTERNAL_DIR}/sensory/TrulyHandsfreeSDK/4.4.23_noexpire/android/lib")
elseif (MACOSX)
    set(VOICE_RECOGNITION_LIB_PATH "${CORETECH_EXTERNAL_DIR}/sensory/TrulyHandsfreeSDK/4.4.23_noexpire/x86_64-darwin/lib")
endif()

set(VOICE_RECOGNITION_LIBS
  thf
)

set(PLATFORM_TAG "")
if (ANDROID)
    set(PLATFORM_TAG "_armeabi-v7a")
endif()

foreach(LIB ${VOICE_RECOGNITION_LIBS})
  add_library(${LIB} STATIC IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${VOICE_RECOGNITION_LIB_PATH}/lib${LIB}${PLATFORM_TAG}.a"
    INTERFACE_INCLUDE_DIRECTORIES
    "${VOICE_RECOGNITION_INCLUDE_PATH}")
endforeach()
