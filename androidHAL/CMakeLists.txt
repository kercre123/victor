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
   -DUSE_ION
   -DAMSS_VERSION
   -D_ANDROID_
   -DCAMERA_ION_HEAP_ID=ION_IOMMU_HEAP_ID
   -DCAMERA_GRALLOC_HEAP_ID=GRALLOC_USAGE_PRIVATE_MM_HEAP
   -DCAMERA_GRALLOC_FALLBACK_HEAP_ID=GRALLOC_USAGE_PRIVATE_IOMMU_HEAP
   -DCAMERA_ION_FALLBACK_HEAP_ID=ION_IOMMU_HEAP_ID
   -DCAMERA_GRALLOC_CACHING_ID=0
   -DNUM_RECORDING_BUFFERS=9
 )

 set_target_properties(proto_camera PROPERTIES LINKER_LANGUAGE C)
 
endif()

include(anki_build_cxx_library)
anki_build_cxx_library(androidHAL ${ANKI_SRCLIST_DIR} STATIC)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
if (ANDROID)
    set(PLATFORM_LIBS
        android
        log
        GLESv2
        mediandk
        camera2ndk
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

