#
# breakpad.cmake
#
set(BREAKPAD_PATH "${CMAKE_SOURCE_DIR}/lib/crash-reporting-vicos/Breakpad")
set(BREAKPAD_INCLUDE_PATHS "${BREAKPAD_PATH}/include")
set(BREAKPAD_LICENSE "BSD-4,${CMAKE_SOURCE_DIR}/licenses/breakpad.license")

# VICOS platform-specific library
set(BREAKPAD_LIBS "")
set(BREAKPAD_EXES "")

if (VICOS)
    set(BREAKPAD_LIB_PATH "${BREAKPAD_PATH}/libs/armeabi-v7a")
    set(BREAKPAD_LIBS
      breakpad_client
    )
    set(BREAKPAD_EXE_PATH "${BREAKPAD_PATH}/bin/armeabi-v7a")
    set(BREAKPAD_EXES
      minidump_stackwalk
    )
endif()

foreach(LIB ${BREAKPAD_LIBS})
  # message(STATUS "LIB: ${LIB}")
  add_library(${LIB} STATIC IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${BREAKPAD_LIB_PATH}/lib${LIB}.a"
    INTERFACE_INCLUDE_DIRECTORIES
    "${BREAKPAD_INCLUDE_PATHS}")
  anki_build_target_license(${LIB} ${BREAKPAD_LICENSE})
endforeach()

foreach(EXE ${BREAKPAD_EXES})
  # message(STATUS "EXE: ${EXE}")
  add_executable(${EXE} IMPORTED)
  set_target_properties(${EXE} PROPERTIES
    IMPORTED_LOCATION
    ${BREAKPAD_EXE_PATH}/${EXE}
  )
  anki_build_target_license(${EXE} ${BREAKPAD_LICENSE})
endforeach()

# Add helper macro to copy exes
macro(copy_breakpad_exes)

  message(STATUS "breakpad exes: ${BREAKPAD_EXES}")

  set(OUTPUT_FILES "")

  foreach(exe ${BREAKPAD_EXES})
      get_target_property(EXE_PATH ${exe} IMPORTED_LOCATION)
      get_filename_component(EXE_NAME ${EXE_PATH} NAME)
      set(DST_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXE_NAME}")
      message(STATUS "copy breakpad exe: ${exe} ${EXE_PATH} -> ${DST_PATH}")
      add_custom_command(
          OUTPUT "${DST_PATH}"
          DEPENDS "${EXE_PATH}"
          COMMAND ${CMAKE_COMMAND}
          ARGS -E copy_if_different "${EXE_PATH}" "${DST_PATH}"
          COMMENT "copy ${EXE_PATH}"
          VERBATIM
      )
      list(APPEND OUTPUT_FILES ${DST_PATH})
  endforeach()

  add_custom_target(copy_breakpad_exes ALL DEPENDS ${OUTPUT_FILES})

endmacro()
