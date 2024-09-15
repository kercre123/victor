set(VOSK_HOME "${ANKI_THIRD_PARTY_DIR}/vosk")
set(VOSK_INCLUDE_PATH "${VOSK_HOME}/vicos/include")

if (VICOS)
    set(VOSK_LIB_PATH "${VOSK_HOME}/vicos/lib")
endif()

set(VOSK_LIBS
  vosk
)

foreach(LIB ${VOSK_LIBS})
  add_library(${LIB} SHARED IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${VOSK_LIB_PATH}/lib${LIB}.so"
    INTERFACE_INCLUDE_DIRECTORIES
    "${VOSK_INCLUDE_PATH}")
  anki_build_target_license(${LIB} "LGPL-2.1,${CMAKE_SOURCE_DIR}/licenses/mpg123.license")
endforeach()

set(INSTALL_LIBS
"${VOSK_LIBS}")

foreach(lib ${VOSK_LIBS})
    get_target_property(LIB_PATH ${lib} IMPORTED_LOCATION)
    get_filename_component(LIB_FILENAME ${LIB_PATH} NAME)
    set(DST_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${LIB_FILENAME}")
    message(STATUS "copy vosk lib: ${lib} ${LIB_PATH} -> ${DST_PATH}")
    add_custom_command(
        OUTPUT ${DST_PATH}
        DEPENDS ${LIB_PATH}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIB_PATH} ${DST_PATH}
        COMMENT "copy ${LIB_PATH}"
        VERBATIM
    )
    list(APPEND OUTPUT_FILES ${DST_PATH})
endforeach()