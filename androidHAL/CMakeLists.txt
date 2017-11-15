cmake_minimum_required(VERSION 3.6)

project(androidHAL)

if (ANDROID)
 
 # This is a library which relies on some kernel headers.
 # ... Because prototyping.
 # They have been checked locally to the camera directory
 
 add_library(proto_camera STATIC
     android/proto_camera/victor_camera.h
     android/proto_camera/victor_camera.c
 )
 
 target_include_directories(proto_camera
   PUBLIC
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
   SYSTEM PUBLIC
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/android/proto_camera/inc>
 )


target_compile_definitions(proto_camera
  PRIVATE
  USE_ION
  AMSS_VERSION
  _ANDROID_
  CAMERA_ION_HEAP_ID=ION_IOMMU_HEAP_ID
  CAMERA_GRALLOC_HEAP_ID=GRALLOC_USAGE_PRIVATE_MM_HEAP
  CAMERA_GRALLOC_FALLBACK_HEAP_ID=GRALLOC_USAGE_PRIVATE_IOMMU_HEAP
  CAMERA_ION_FALLBACK_HEAP_ID=ION_IOMMU_HEAP_ID
  CAMERA_GRALLOC_CACHING_ID=0
  NUM_RECORDING_BUFFERS=9
)

target_compile_options(proto_camera
PRIVATE
  -O3
)

set_target_properties(proto_camera PROPERTIES LINKER_LANGUAGE C)
 
endif()

include(anki_build_cxx)

anki_build_cxx_library(androidHAL ${ANKI_SRCLIST_DIR})

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
if (ANDROID)
    set(PLATFORM_LIBS
        android
        log
        GLESv2
        proto_camera)
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
  ${PLATFORM_LIBS}
)

android_strip(TARGET androidHAL)

target_compile_definitions(androidHAL
  PRIVATE
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(androidHAL
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> # allow "android/" prefix
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../robot/include>
  ${PLATFORM_INCLUDES}
)
