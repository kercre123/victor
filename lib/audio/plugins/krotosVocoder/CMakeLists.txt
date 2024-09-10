cmake_minimum_required(VERSION 3.6)

project(krotosVocoder)

anki_build_cxx_library(krotos_vocoder ${ANKI_SRCLIST_DIR} STATIC)

target_compile_options(krotos_vocoder
  PRIVATE
  -Wno-deprecated-declarations
  -Wno-undef
  -DORBIS # Stop from redefining M_PI
)

target_include_directories(krotos_vocoder
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/WwisePlugin>
  PRIVATE
  $<BUILD_INTERFACE:${WWISE_SDK_ROOT}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/KrotosDSP>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/PitchTracking>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/PitchTracking/audio-analysis-framework>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/PitchTracking/audio-analysis-framework/libraries>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/PitchTracking/PitchTrackingAlgorithms>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/Vocoder>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/VocoderFX>
)

anki_build_target_license(krotos_vocoder "Commercial")

anki_build_strip(TARGET krotos_vocoder)

