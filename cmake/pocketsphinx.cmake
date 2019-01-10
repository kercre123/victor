set(POCKETSPHINX_HOME "${ANKI_EXTERNAL_DIR}/anki-thirdparty/pocketsphinx")
set(POCKETSPHINX_INCLUDE_PATH "${POCKETSPHINX_HOME}/include/pocketsphinx")

# TODO: no mac libs yet
# if (VICOS)
    set(POCKETSPHINX_LIB_PATH "${POCKETSPHINX_HOME}/lib")
# elseif (MACOSX)
#     set(POCKETSPHINX_LIB_PATH "${POCKETSPHINX_HOME}/...")
# endif()

if (VICOS)

set(POCKETSPHINX_LIBS
  pocketsphinx
)

foreach(LIB ${POCKETSPHINX_LIBS})
  add_library(${LIB} SHARED IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
      "${POCKETSPHINX_LIB_PATH}/lib${LIB}.so.3"
    INTERFACE_INCLUDE_DIRECTORIES
      "${POCKETSPHINX_INCLUDE_PATH}"
  )

  # anki_build_target_license(${LIB} "TODO")
endforeach()

foreach(lib ${POCKETSPHINX_LIBS})
    get_target_property(libimport ${lib} IMPORTED_LOCATION)
    get_filename_component(libname ${libimport} NAME)
    set(libpath "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${libname}")
    add_custom_command(
        OUTPUT ${libpath}
        DEPENDS ${libimport}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${libimport} ${libpath}
        COMMENT "install ${libname}"
        VERBATIM
    )
    list(APPEND POCKETSPHINX_LIB_NAMES ${libname})
    list(APPEND POCKETSPHINX_LIB_PATHS ${libpath})
endforeach()

add_custom_target(copy_pocketsphinx_libs ALL DEPENDS ${POCKETSPHINX_LIB_PATHS})

endif()