if (VICOS)
  set(LIBAUBIO_INCLUDE_PATH "${ANKI_EXTERNAL_DIR}/aubio/vicos/include")
  set(LIBAUBIO_LIB_PATH "${ANKI_EXTERNAL_DIR}/aubio/vicos/lib/libaubio.a")
elseif (MACOSX)
  set(LIBAUBIO_INCLUDE_PATH "${ANKI_EXTERNAL_DIR}/aubio/mac/include")
  set(LIBAUBIO_LIB_PATH "${ANKI_EXTERNAL_DIR}/aubio/mac/lib/libaubio.a")
endif()

set(AUBIO_LIBS aubio)

add_library(aubio STATIC IMPORTED)
anki_build_target_license(aubio "Commercial")

set_target_properties(aubio PROPERTIES
  IMPORTED_LOCATION "${LIBAUBIO_LIB_PATH}"
  INTERFACE_INCLUDE_DIRECTORIES "${LIBAUBIO_INCLUDE_PATH}")
