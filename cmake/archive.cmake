
set(ANKI_HAS_LIBARCHIVE "FALSE")
set(LIBARCHIVE_INCLUDE_PATH "")
set(LIBARCHIVE_LIBS "")

if (ANKI_HAS_LIBARCHIVE)
  set(LIBARCHIVE_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/libarchive/project/include")

  if (ANDROID)
    set(LIBARCHIVE_LIB_PATH "${CORETECH_EXTERNAL_DIR}/libarchive/project/android/DerivedData")
  elseif (MACOSX)
    set(LIBARCHIVE_LIB_PATH "${CORETECH_EXTERNAL_DIR}/libarchive/project/mac/DerivedData/Release")
  endif()

  set(LIBARCHIVE_LIBS archive)

  add_library(archive STATIC IMPORTED)
  set_target_properties(archive PROPERTIES
    IMPORTED_LOCATION "${LIBARCHIVE_LIB_PATH}/libarchive.a"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBARCHIVE_INCLUDE_PATH}")
endif()
