if(PFFFT_INCLUDED)
  return()
endif(PFFFT_INCLUDED)
set(PFFFT_INCLUDED true)

if (VICOS)
  set(LIBPFFFT_INCLUDE_PATH "${ANKI_EXTERNAL_DIR}/pffft/vicos/include")
  set(LIBPFFFT_LIB_PATH "${ANKI_EXTERNAL_DIR}/pffft/vicos/lib")
elseif (MACOSX)
  set(LIBPFFFT_INCLUDE_PATH "${ANKI_EXTERNAL_DIR}/pffft/mac/include")
  set(LIBPFFFT_LIB_PATH "${ANKI_EXTERNAL_DIR}/pffft/mac/lib")
endif()


set(PFFFT_LIBS
  pffft
)

foreach(LIB ${PFFFT_LIBS})
  add_library(${LIB} STATIC IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${LIBPFFFT_LIB_PATH}/lib${LIB}.a"
    INTERFACE_INCLUDE_DIRECTORIES
    "${LIBPFFFT_INCLUDE_PATH}")
  anki_build_target_license(${LIB} "FFTPACK5,${CMAKE_SOURCE_DIR}/licenses/pffft.license")
endforeach()

if (TARGET copy_pffft_libs)
    return()
endif()

message(STATUS "pffft libs: ${PFFFT_LIBS}")

set(OUTPUT_FILES "")

foreach(lib ${PFFFT_LIBS})
    get_target_property(LIB_PATH ${lib} IMPORTED_LOCATION)
    get_filename_component(LIB_FILENAME ${LIB_PATH} NAME)
    set(DST_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${LIB_FILENAME}")
    message(STATUS "copy pffft lib: ${lib} ${LIB_PATH} -> ${DST_PATH}")
    add_custom_command(
        OUTPUT ${DST_PATH}
        DEPENDS ${LIB_PATH}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIB_PATH} ${DST_PATH}
        COMMENT "copy ${LIB_PATH}"
        VERBATIM
    )
    list(APPEND OUTPUT_FILES ${DST_PATH})
endforeach()

add_custom_target(copy_pffft_libs ALL DEPENDS ${OUTPUT_FILES})
