set(PRYON_LITE_HOME "${ANKI_EXTERNAL_DIR}/anki-thirdparty/pryon_lite")
set(PRYON_LITE_INCLUDE_PATH "${PRYON_LITE_HOME}/armv7a-anki")

# TODO: Waiting for Amazon to provide Libs for Mac platform
# if (VICOS)
    set(PRYON_LITE_LIB_PATH "${PRYON_LITE_HOME}/armv7a-anki")
# elseif (MACOSX)
#     set(PRYON_LITE_LIB_PATH "${PRYON_LITE_HOME}/...")
# endif()

if (VICOS)

set(PRYON_LITE_LIBS
  pryon_lite
)

foreach(LIB ${PRYON_LITE_LIBS})
  add_library(${LIB} STATIC IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${PRYON_LITE_LIB_PATH}/lib${LIB}.a"
    INTERFACE_INCLUDE_DIRECTORIES
    "${PRYON_LITE_INCLUDE_PATH}")
  # anki_build_target_license(${LIB} "Commercial")
endforeach()

endif()
