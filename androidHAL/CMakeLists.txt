cmake_minimum_required(VERSION 3.6)
project(androidHAL)

# HACK: fugly_camera
if (ANDROID)

# This is a library generated from an object file that
# comes from a pre-processed .c file... Because prototyping.
# Since the .c file is pre-compiled with gcc instead of clang,
# the easiest way we can actually use is by pre-compiling
# the object file and making a lib out of it.
# This is super fucking ugly, but it should let us get images into the engine

add_library(fugly_camera STATIC
    android/fugly_camera/victor_camera.h
    android/fugly_camera/processed_victor_camera.o
)

target_include_directories(fugly_camera
PUBLIC
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

set_target_properties(fugly_camera PROPERTIES LINKER_LANGUAGE C)

endif()
#
# end fugly_camera hack
#


include(anki_build_cxx_library)
anki_build_cxx_library(androidHAL ${ANKI_SRCLIST_DIR} STATIC)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
if (ANDROID)
    set(PLATFORM_LIBS
        fugly_camera
        android
        log
        GLESv2
        mediandk
        camera2ndk)
elseif (MACOSX)
    include(webots)
    set(PLATFORM_LIBS
        ${OPENCV_LIBS}
        ${WEBOTS_LIBS}
    )
    set(PLATFORM_COMPILE_DEFS "-DSIMULATOR")
endif()

target_link_libraries(androidHAL
PRIVATE
  util
# cti
  cti_common
  cti_vision
  cti_planning
  cti_messaging
  robot_clad # imageTypes.h
  # platform
#PUBLIC
  # these need to be public until we remove android-specific classes from androidHAL.h
  ${PLATFORM_LIBS}
)

android_strip(TARGET androidHAL)

target_compile_definitions(androidHAL
  PRIVATE
  -DCOZMO_BASESTATION
  -DCOZMO_V2
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(androidHAL PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> # allow "android/" prefix
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../robot/include>
  ${PLATFORM_INCLUDES}
)

