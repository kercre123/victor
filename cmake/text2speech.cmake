set(TEXT2SPEECH_HOME "${ANKI_EXTERNAL_DIR}/anki-thirdparty/acapela")
set(TEXT2SPEECH_INCLUDE_PATH "")

set(PLATFORM_SUFFIX "")
if (ANDROID)
  set(PLATFORM_SUFFIX "android")
  set(TEXT2SPEECH_LIB_PATH "${TEXT2SPEECH_HOME}/AcapelaTTS_for_Android_V1.612/sdk/armeabi-v7a")
elseif (MACOSX)
  # TTS (libacatts.dylib) should not be linked directly on mac
  # set(TEXT2SPEECH_LIB_PATH "${TEXT2SPEECH_HOME}/AcapelaTTS_for_Mac_V9.450")
  set(TEXT2SPEECH_INCLUDE_PATH "${TEXT2SPEECH_HOME}/AcapelaTTS_for_Mac_V9.450/SDK/Include")
endif()

set(TEXT2SPEECH_LIBS
  acatts
)

foreach(LIB ${TEXT2SPEECH_LIBS})
  if (TEXT2SPEECH_LIB_PATH)
      add_library(${LIB} SHARED IMPORTED)
      set(LIB_PATH "${TEXT2SPEECH_LIB_PATH}/lib${LIB}${PLATFORM_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}")
      set_target_properties(${LIB} PROPERTIES
        IMPORTED_LOCATION
        ${LIB_PATH}
        INTERFACE_INCLUDE_DIRECTORIES
        "${TEXT2SPEECH_INCLUDE_PATH}")
  else()
      add_library(${LIB} INTERFACE IMPORTED)
      set_target_properties(${LIB} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
        "${TEXT2SPEECH_INCLUDE_PATH}")
  endif()
endforeach()

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
