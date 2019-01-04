set(SPHINXBASE_HOME "${ANKI_EXTERNAL_DIR}/anki-thirdparty/sphinxbase")
set(SPHINXBASE_INCLUDE_PATH "${SPHINXBASE_HOME}/include")

# TODO: no mac libs yet
# if (VICOS)
    set(SPHINXBASE_LIB_PATH "${SPHINXBASE_HOME}/lib")
# elseif (MACOSX)
#     set(SPHINXBASE_LIB_PATH "${SPHINXBASE_HOME}/...")
# endif()

if (VICOS)

set(SPHINXBASE_LIBS
  sphinxbase
  sphinxad
)

foreach(LIB ${SPHINXBASE_LIBS})
  add_library(${LIB} STATIC IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${SPHINXBASE_LIB_PATH}/lib${LIB}.so.3"
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
    "${SPHINXBASE_INCLUDE_PATH}/sphinxbase"

    INTERFACE_INCLUDE_DIRECTORIES
    "${SPHINXBASE_INCLUDE_PATH}"
  )
  # anki_build_target_license(${LIB} "TODO")
endforeach()

foreach(lib ${SPHINXBASE_LIBS})
    get_target_property(libimport ${lib} IMPORTED_LOCATION)
    get_filename_component(libname ${libimport} NAME)
    set(libpath "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${libname}")
    add_custom_command(
        OUTPUT ${libpath}
        DEPENDS ${libimport}
        # note: lib provides .so.3.0.0, but libpocketsphinx requests .so.3
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${libimport}.0.0 ${libpath}
        COMMENT "install ${libname}"
        VERBATIM
    )
    list(APPEND SPHINXBASE_LIB_NAMES ${libname})
    list(APPEND SPHINXBASE_LIB_PATHS ${libpath})
endforeach()

add_custom_target(copy_sphinxbase_libs ALL DEPENDS ${SPHINXBASE_LIB_PATHS})


endif()