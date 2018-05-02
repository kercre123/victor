set(BREAKPAD_INCLUDE_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# FIXME: We need to build Breakpad libs for VICOS
# Android platform-specific library
set(BREAKPAD_LIBS "")

if (ANDROID)
    set(BREAKPAD_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/armeabi-v7a")
    set(BREAKPAD_LIBS
      breakpad_client
    )
endif()

foreach(LIB ${BREAKPAD_LIBS})
    add_library(${LIB} STATIC IMPORTED)
    set_target_properties(${LIB} PROPERTIES
        IMPORTED_LOCATION
        "${BREAKPAD_LIB_PATH}/lib${LIB}.a"
        INTERFACE_INCLUDE_DIRECTORIES
        "${BREAKPAD_INCLUDE_PATHS}")
    message(STATUS "LIB: ${LIB}")
endforeach()
