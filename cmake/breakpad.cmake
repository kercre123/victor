set(BREAKPAD_INCLUDE_PATHS
    "${CMAKE_SOURCE_DIR}/lib/crash-reporting-vicos/Breakpad/include"
)

# VICOS platform-specific library
set(BREAKPAD_LIBS "")

if (VICOS)
    set(BREAKPAD_LIB_PATH "${CMAKE_SOURCE_DIR}/lib/crash-reporting-vicos/Breakpad/libs/armeabi-v7a")
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
    anki_build_target_license(${LIB} "BSD-4,${CMAKE_SOURCE_DIR}/licenses/breakpad.license")
    # message(STATUS "LIB: ${LIB}")
endforeach()

#if (NOT breakpad_client)
#  message(FATAL_ERROR "breakpad_client target not added")
#endif()
