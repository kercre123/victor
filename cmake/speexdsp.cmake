set(SPEEXDSP_HOME "${ANKI_EXTERNAL_DIR}/coretech_external/speexdsp-1.2rc3")

set(PLATFORM_DIR "")

if (ANDROID)
  set(PLATFORM_DIR "android")
elseif (MACOSX)
  set(PLATFORM_DIR "mac")
endif()

set(SPEEXDSP_LIB_PATH "${SPEEXDSP_HOME}/project/${PLATFORM_DIR}/lib")
set(SPEEXDSP_INCLUDE_PATHS "${SPEEXDSP_HOME}/project/${PLATFORM_DIR}/include")
set(SPEEXDSP_LIBS "speexdsp")

foreach(LIB ${SPEEXDSP_LIBS})
    add_library(${LIB} STATIC IMPORTED)
    set_target_properties(${LIB} PROPERTIES
        IMPORTED_LOCATION
        "${SPEEXDSP_LIB_PATH}/lib${LIB}.a"
        INTERFACE_INCLUDE_DIRECTORIES
        "${SPEEXDSP_INCLUDE_PATHS}")
endforeach()
