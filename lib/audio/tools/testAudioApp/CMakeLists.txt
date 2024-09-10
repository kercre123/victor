cmake_minimum_required(VERSION 3.6)

project(test_audio_app)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)

add_executable(test_audio_app
  "${SOURCE_DIR}/testAudioMain.cpp"
  "${SOURCE_DIR}/testAudioLogger.h"
  "${SOURCE_DIR}/testAudioEngineController.h"
  "${SOURCE_DIR}/testAudioEngineController.cpp"
)

set(TEST_AUDIO_APP_COMPILE_OPTIONS
  ${ANKI_BUILD_CXX_COMPILE_OPTIONS}
)

target_link_libraries(test_audio_app
  audio_engine
  util
  ${PLATFORM_LIBS}
)

anki_build_strip(TARGET test_audio_app)

target_compile_definitions(test_audio_app
  PRIVATE
  ${ANKI_LIB_AUDIO_PLUGINS}
  ${ANKI_BUILD_CXX_COMPILE_DEFINITIONS}
)

target_compile_options(test_audio_app
  PRIVATE
  ${TEST_AUDIO_APP_COMPILE_OPTIONS}
)
