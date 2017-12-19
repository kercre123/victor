
if (MACOSX)
  set(AUBIO_INCLUDE_PATH "/usr/local/Cellar/aubio/0.4.6/include")
  set(AUBIO_LIB_PATH "/usr/local/Cellar/aubio/0.4.6/lib/libaubio.dylib")
elseif (ANDROID)
  set(AUBIO_INCLUDE_PATH "/Users/matt/github/aubio/dist-android-19-arm/usr/local/include")
  set(AUBIO_LIB_PATH "/Users/matt/github/aubio/dist-android-19-arm/usr/local/lib/libaubio.so")
endif()

set(AUBIO_LIBS aubio)

foreach(LIB ${AUBIO_LIBS})
  add_library(${LIB} SHARED IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${AUBIO_LIB_PATH}"
    INTERFACE_INCLUDE_DIRECTORIES
    "${AUBIO_INCLUDE_PATH}")
endforeach()

