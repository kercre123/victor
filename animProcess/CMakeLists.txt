cmake_minimum_required(VERSION 3.6)

project(animProcess)

# Build options
option(USE_DAS "Enable DAS reporting" OFF)

include(anki_build_cxx)
include(aubio)
include(flatbuffers)
include(opencv)
include(text2speech)
include(sensory)
include(robot_core)

anki_build_cxx_library(victor_anim ${ANKI_SRCLIST_DIR} STATIC)
anki_build_target_license(victor_anim "ANKI" "Public Domain,${CMAKE_SOURCE_DIR}/lib/util/source/3rd/gif-h/LICENSE" "Public Domain,${CMAKE_SOURCE_DIR}/lib/util/source/3rd/jo_gif/jo_gif.h")
set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES ${OPENCV_INCLUDE_PATHS})
set(PLATFORM_COMPILE_DEFS "")

if (VICOS)
    set(PLATFORM_LIBS
        log
        robot_core)
elseif (MACOSX)
    include(webots)
    set(PLATFORM_LIBS
        ${OPENCV_LIBS}
        ${WEBOTS_LIBS}
    )
    set(PLATFORM_COMPILE_DEFS "-DSIMULATOR")
endif()

target_link_libraries(victor_anim
  PUBLIC
  victor_web_library
  robot_interface  # Needs to be public for cozmoConfig.h
  PRIVATE
  util
  util_audio
  canned_anim_lib_anim
  # cti
  cti_common_robot
  cti_vision
  cti_messaging_robot
  robot_clad_cpplite
  robot_transport
  cloud_clad_cpp
  audio_engine
  audio_multiplexer_robot
  osState
  micdata
  # vendor
  ${OPENCV_LIBS}
  ${FLATBUFFERS_LIBS}
  signal_essence
  ${TEXT2SPEECH_LIBS}
  ${SENSORY_LIBS}
  # platform
  ${PLATFORM_LIBS}
  # Aubio beat detection
  ${AUBIO_LIBS}
  PUBLIC
  victor_web_library
  robot_interface  # Needs to be public for cozmoConfig.h
)

anki_build_strip(TARGET victor_anim)

target_compile_definitions(victor_anim
  PRIVATE
  USE_DAS=$<BOOL:${USE_DAS}>
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(victor_anim
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/lib/util/source/3rd>
  ${PLATFORM_INCLUDES}
)

# victor_anim binary only builds on vicos now
# mac implementation uses (webotsCtrlAnim)
if (VICOS)

  anki_build_cxx_executable(vic-anim ${ANKI_SRCLIST_DIR})
  anki_build_target_license(vic-anim "ANKI")

  target_link_libraries(vic-anim
    PRIVATE
    victor_anim
    victorCrashReports
    # NOTE: deliberately using non-robot cti_common here to ease build
    #       pain given differing include paths for clad types. This still
    #       does not expose cpplite clad messaging within the anim process 
    #       (which is undesirable) since this is limited just to the 
    #       vic-anim executable (vs. using cti_common_robot and thus 
    #       cpplite clad in the victor_anim library above). Ideally this
    #       separation would be clearer and cleaner or we would 
    #       exclusively use one or the other, but that will have to 
    #       wait for a (much) larger cleanup to cmake / clad / coretech.
    cti_common
    jsoncpp
    ${PLATFORM_LIBS}
  )

  anki_build_strip(TARGET vic-anim)

endif()
