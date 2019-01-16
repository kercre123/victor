set(PICOVOICE_HOME "${CMAKE_SOURCE_DIR}/lib/picovoice") 

if (VICOS)
  set(PICOVOICE_LIB_TYPE SHARED)
  set(PICOVOICE_LIB "${PICOVOICE_HOME}/lib/vicos/libpv_porcupine_softfp.so")
elseif (MACOSX)
  set(PICOVOICE_LIB_TYPE STATIC)
  set(PICOVOICE_LIB "${PICOVOICE_HOME}/lib/mac/libpv_porcupine.a")
endif()

add_library(pv_porcupine ${PICOVOICE_LIB_TYPE} IMPORTED GLOBAL)
set_target_properties(pv_porcupine PROPERTIES
  IMPORTED_LOCATION
    "${PICOVOICE_LIB}"
  INTERFACE_INCLUDE_DIRECTORIES
    "${PICOVOICE_HOME}/include"
)

string(COMPARE EQUAL "${build_system}" SHARED _is_shared_lib)

if (_is_shared_lib)
  if (NOT TARGET copy_picovoice_libs)

  set(OUTPUT_FILES "")
  set(INSTALL_LIBS pv_porcupine)
  foreach(lib ${INSTALL_LIBS})
      get_target_property(LIB_PATH ${lib} IMPORTED_LOCATION)
      get_filename_component(LIB_FILENAME ${LIB_PATH} NAME)
      set(DST_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${LIB_FILENAME}") 
      message(STATUS "copy picovoice lib: ${lib} ${LIB_PATH} -> ${DST_PATH}")
      add_custom_command(
        OUTPUT "${DST_PATH}"
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy_if_different "${LIB_PATH}" "${DST_PATH}"
        COMMENT "copy ${LIB_PATH}"
        VERBATIM
      )
      list(APPEND OUTPUT_FILES ${DST_PATH})
  endforeach() 

  add_custom_target(copy_picovoice_libs ALL DEPENDS ${OUTPUT_FILES})

  #anki_build_target_license(pv_porcupine "Commercial")
  endif(NOT TARGET copy_picovoice_libs)
endif(_is_shared_lib)
