set(AUBIO_HOME "/Users/matt/github/aubio")

set(AUBIO_INCLUDE_PATH
   "${AUBIO_HOME}/build/dist/usr/local/include"
)

set(AUBIO_LIBS
  aubio
)


if (MACOSX)
  set(AUBIO_LIB_PATH "${AUBIO_HOME}/build/dist/usr/local/lib")
endif()

foreach(LIB ${AUBIO_LIBS})
  add_library(${LIB} STATIC IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${AUBIO_LIB_PATH}/lib${LIB}.a"
    INTERFACE_INCLUDE_DIRECTORIES
    "${AUBIO_INCLUDE_PATH}")
endforeach()
