#
# cmake directories for use with Acapela TTS SDK
#
# Path to third-party repo
set(TEXT2SPEECH_HOME "${ANKI_EXTERNAL_DIR}/anki-thirdparty/acapela")

# Path to third-party header files for this platform
set(TEXT2SPEECH_INCLUDE_DIRS "")

# Path to third-party library files for this platform
set(TEXT2SPEECH_LIB_DIR "")

# List of library targets for this platform
set(TEXT2SPEECH_LIBS "")

if (VICOS)

  #
  # Use Acapela TTS for Linux Embedded V8.511
  # Use static linking with libbabile.a
  #
  # Note that libbabile.a relies on some glibc functions
  # (__error_location, __isoc99_vssprintf) that are not
  # supported by clang.  We define our own version of 
  # these functions to satisfy the linker.
  #

  set(TEXT2SPEECH_SDK "${TEXT2SPEECH_HOME}/AcapelaTTS_for_LinuxEmbedded_V8.511")
  set(TEXT2SPEECH_INCLUDE_DIRS
    "${TEXT2SPEECH_SDK}/sdk/babile/include"
    "${TEXT2SPEECH_SDK}/sdk/babile/sample/simpledemo"
  )
  set(TEXT2SPEECH_LIB_DIR "${TEXT2SPEECH_SDK}/libraries/arm7l-gcc-4.8.3-gnueabi")
  set(TEXT2SPEECH_LIBS babile)

  foreach(lib ${TEXT2SPEECH_LIBS})
    add_library(${lib} STATIC IMPORTED)
    set_target_properties(${lib}
      PROPERTIES
      IMPORTED_LOCATION "${TEXT2SPEECH_LIB_DIR}/lib${lib}.a"
      INTERFACE_INCLUDE_DIRECTORIES "${TEXT2SPEECH_INCLUDE_DIRS}"
    )
  endforeach()

elseif (MACOSX)

  #
  # Use Acapela TTS for Mac V9.450
  # Use dynamic linking with libacatts.dylib
  #
  # Note that libacatts.dylib is NOT linked directly into executables!
  # It is loaded programmatically by Acapela header magic.  We create
  # a symbolic link (resources/tts/libacatts.dylib) so the library can be 
  # located at runtime.
  #
  
  set(TEXT2SPEECH_SDK "${TEXT2SPEECH_HOME}/AcapelaTTS_for_Mac_V9.450")
  set(TEXT2SPEECH_INCLUDE_DIRS "${TEXT2SPEECH_SDK}/SDK/Include")
  set(TEXT2SPEECH_LIB_DIR "${TEXT2SPEECH_SDK}")
  set(TEXT2SPEECH_LIBS acatts)

  foreach(lib ${TEXT2SPEECH_LIBS})
    add_library(${lib} INTERFACE)
    set_target_properties(${lib}
      PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${TEXT2SPEECH_INCLUDE_DIRS}"
  )
  endforeach()

endif()

macro(copy_tts_android_libs)
  set(INSTALL_LIBS ${TEXT2SPEECH_LIBS})
  #message(STATUS "tts libs: ${INSTALL_LIBS}")
  set(OUTPUT_FILES "")
  foreach(lib ${INSTALL_LIBS})
    get_target_property(LIB_PATH ${lib} IMPORTED_LOCATION)
    get_filename_component(LIB_FILENAME ${LIB_PATH} NAME)
    set(DST_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${LIB_FILENAME}")
    #message(STATUS "copy lib: ${lib} ${LIB_PATH} -> ${DST_PATH}")
    add_custom_command(
      OUTPUT "${DST_PATH}"
      COMMAND ${CMAKE_COMMAND}
        ARGS -E copy_if_different "${LIB_PATH}" "${DST_PATH}"
        COMMENT "copy ${LIB_PATH}"
        VERBATIM
      )
      list(APPEND OUTPUT_FILES ${DST_PATH})
    endforeach()
    add_custom_target(copy_tts_libs ALL DEPENDS ${OUTPUT_FILES})
endmacro()
