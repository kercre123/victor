if (ANDROID)
  set(LIBDLIB_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/build/dlib-linux/include")
  set(LIBDLIB_LIB_PATH "${CORETECH_EXTERNAL_DIR}/build/dlib-linux/lib/libdlib.a")
elseif (VICOS)
  set(LIBDLIB_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/build/dlib-vicos/include")
  set(LIBDLIB_LIB_PATH "${CORETECH_EXTERNAL_DIR}/build/dlib-vicos/lib/libdlib.a")
elseif (MACOSX)
  set(LIBDLIB_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/build/dlib-mac/include")
  set(LIBDLIB_LIB_PATH "${CORETECH_EXTERNAL_DIR}/build/dlib-mac/lib/libdlib.a")
endif()

set(DLIB_LIBS dlib)

add_library(dlib STATIC IMPORTED)
anki_build_target_license(dlib "Boost,${CMAKE_SOURCE_DIR}/licenses/dlib.license")

set_target_properties(dlib PROPERTIES
  IMPORTED_LOCATION "${LIBDLIB_LIB_PATH}"
  INTERFACE_INCLUDE_DIRECTORIES "${LIBDLIB_INCLUDE_PATH}")
