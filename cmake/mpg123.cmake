if(MPG123_INCLUDED)
  return()
endif(MPG123_INCLUDED)
set(MPG123_INCLUDED true)

if (VICOS)
  set(LIBMPG123_INCLUDE_PATH "${ANKI_EXTERNAL_DIR}/mpg123/vicos/include")
  set(LIBMPG123_LIB_PATH "${ANKI_EXTERNAL_DIR}/mpg123/vicos/lib")
  set(LIBMPG123_EXT "so.0")
elseif (MACOSX)
  set(LIBMPG123_INCLUDE_PATH "${ANKI_EXTERNAL_DIR}/mpg123/mac/include")
  set(LIBMPG123_LIB_PATH "${ANKI_EXTERNAL_DIR}/mpg123/mac/lib")
  set(LIBMPG123_EXT "0.dylib")
endif()


set(MPG123_LIBS
  mpg123
)

foreach(LIB ${MPG123_LIBS})
  add_library(${LIB} SHARED IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${LIBMPG123_LIB_PATH}/lib${LIB}.${LIBMPG123_EXT}"
    INTERFACE_INCLUDE_DIRECTORIES
    "${LIBMPG123_INCLUDE_PATH}")
  anki_build_target_license(${LIB} "LGPL-2.1,${CMAKE_SOURCE_DIR}/licenses/mpg123.license")
endforeach()

if (TARGET copy_mpg123_libs)
    return()
endif()

message(STATUS "mpg123 libs: ${MPG123_LIBS}")

set(OUTPUT_FILES "")

foreach(lib ${MPG123_LIBS})
    get_target_property(LIB_PATH ${lib} IMPORTED_LOCATION)
    get_filename_component(LIB_FILENAME ${LIB_PATH} NAME)
    set(DST_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${LIB_FILENAME}")
    message(STATUS "copy mpg123 lib: ${lib} ${LIB_PATH} -> ${DST_PATH}")
    add_custom_command(
        OUTPUT ${DST_PATH}
        DEPENDS ${LIB_PATH}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIB_PATH} ${DST_PATH}
        COMMENT "copy ${LIB_PATH}"
        VERBATIM
    )
    list(APPEND OUTPUT_FILES ${DST_PATH})
endforeach()

add_custom_target(copy_mpg123_libs ALL DEPENDS ${OUTPUT_FILES})
