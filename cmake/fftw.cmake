# Turns out what I read as "MIT License" was actually "MIT Licensing Office," and this is GPLd. We'll need something else.
# ALEXA ONLY!

if(FFTW_INCLUDED)
  return()
endif(FFTW_INCLUDED)
set(FFTW_INCLUDED true)

if (VICOS)
  set(LIBFFTW_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/build/fftw/vicos/include")
  set(LIBFFTW_LIB_PATH "${CORETECH_EXTERNAL_DIR}/build/fftw/vicos/lib")
elseif (MACOSX)
  set(LIBFFTW_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/build/fftw/mac/include")
  set(LIBFFTW_LIB_PATH "${CORETECH_EXTERNAL_DIR}/build/fftw/mac/lib")
endif()


set(FFTW_LIBS
  # Select one precision, single (fftwf) or double (fftw), or both if you really want.
  # Note that NEON instructions are omitted from double precision builds. This is a requirement
  # for 32bit ARM platforms, and is done in mac builds as well for symmetry.
  fftw3f
  # fftw3
)

foreach(LIB ${FFTW_LIBS})
  add_library(${LIB} STATIC IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${LIBFFTW_LIB_PATH}/lib${LIB}.a"
    INTERFACE_INCLUDE_DIRECTORIES
    "${LIBFFTW_INCLUDE_PATH}")
  anki_build_target_license(${LIB} "turns out this is GPLd :(. Will find a new lib,${CMAKE_SOURCE_DIR}/licenses/fftw3.license")
endforeach()

if (TARGET copy_fftw_libs)
    return()
endif()

set(INSTALL_LIBS
  "${FFTW_LIBS}")

message(STATUS "fftw libs: ${INSTALL_LIBS}")

set(OUTPUT_FILES "")

foreach(lib ${INSTALL_LIBS})
    get_target_property(LIB_PATH ${lib} IMPORTED_LOCATION)
    get_filename_component(LIB_FILENAME ${LIB_PATH} NAME)
    set(DST_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${LIB_FILENAME}") 
    message(STATUS "copy fftw lib: ${lib} ${LIB_PATH} -> ${DST_PATH}")
    add_custom_command(
        OUTPUT "${DST_PATH}"
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy_if_different "${LIB_PATH}" "${DST_PATH}"
        COMMENT "copy ${LIB_PATH}"
        VERBATIM
    )
    list(APPEND OUTPUT_FILES ${DST_PATH})
endforeach() 

add_custom_target(copy_fftw_libs ALL DEPENDS ${OUTPUT_FILES})
