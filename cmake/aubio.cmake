if (ANDROID)
  set(LIBAUBIO_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/build/aubio-linux/include")
  set(LIBAUBIO_LIB_PATH "${CORETECH_EXTERNAL_DIR}/build/aubio-linux/lib/libaubio.a")
elseif (VICOS)
  set(LIBAUBIO_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/build/aubio-vicos/include")
  set(LIBAUBIO_LIB_PATH "${CORETECH_EXTERNAL_DIR}/build/aubio-vicos/lib/libaubio.a")
elseif (MACOSX)
  set(LIBAUBIO_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/build/aubio-mac/include")
  set(LIBAUBIO_LIB_PATH "${CORETECH_EXTERNAL_DIR}/build/aubio-mac/lib/libaubio.a")
endif()

set(AUBIO_LIBS aubio)

add_library(aubio STATIC IMPORTED)

set_target_properties(aubio PROPERTIES
  IMPORTED_LOCATION "${LIBAUBIO_LIB_PATH}"
  INTERFACE_INCLUDE_DIRECTORIES "${LIBAUBIO_INCLUDE_PATH}")
