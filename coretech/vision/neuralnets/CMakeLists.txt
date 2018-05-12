# coretech/vision/neuralnets/CMakeLists.txt

cmake_minimum_required(VERSION 3.6)

message(STATUS "Building NeuralNets process")

include(anki_build_cxx)
include(anki_build_strip)

set(TENSORFLOW_PATH  ${CORETECH_EXTERNAL_DIR}/tensorflow)
set(VICTOR_REPO_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_FLAGS "")

if (VICOS)

  set(PLATFORM_FLAGS
    --std=c++11 
    -stdlib=libc++    # NOTE: VicOS uses libc++, so TensorFlow must be built to match
    -fPIE 
    -mfloat-abi=softfp 
    -mfpu=neon 
    -pie 
  )
  
  set(PLATFORM_INCLUDES
    ${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/vicos/include
  )

  set(PLATFORM_LIBS
    -Wl,--whole-archive 
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen_VICOS/lib/libtensorflow-core.a
    -Wl,--no-whole-archive
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen_VICOS/protobuf/armeabi-v7a/lib/libprotobuf.a 
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/downloads/nsync/builds/default.vicos.c++11/libnsync.a
    c++   # NOTE: VicOS uses libc++, so TensorFlow must be built to match
    dl
    log
    z
  )

elseif (MACOSX)
  
  include(webots)

  set(PLATFORM_FLAGS
    --std=c++11 
    -DSIMULATOR
  )
  
  set(PLATFORM_INCLUDES
    ${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac
  )

  set(PLATFORM_LIBS
    -Wl,-force_load
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/lib/libtensorflow-core.a
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/protobuf/lib/libprotobuf.a
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/downloads/nsync/builds/default.macos.c++11/libnsync.a
    ${WEBOTS_LIBS}
    stdc++
    m
  )

endif()

anki_build_cxx_executable(vic-neuralnets ${ANKI_SRCLIST_DIR})

target_compile_definitions(vic-neuralnets 
  PRIVATE
  TENSORFLOW
)

target_compile_options(vic-neuralnets 
  PRIVATE 
  -Wno-undef            # Tensorflow includes don't adhere to our lofty standards
  -Wno-shorten-64-to-32 #  "
  $<$<CONFIG:Debug>:-O0>
  $<$<CONFIG:Release>:-O2>
  ${SYMBOL_VISIBILITY_FLAGS}
  ${PLATFORM_FLAGS}
)

target_include_directories(vic-neuralnets 
  PRIVATE
  ${PLATFORM_INCLUDES}
  ${TENSORFLOW_PATH}
  ${TENSORFLOW_PATH}/bazel-tensorflow/external/eigen_archive
  ${TENSORFLOW_PATH}/bazel-genfiles
  ${TENSORFLOW_PATH}/bazel-tensorflow/external/protobuf_archive/src
  ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/downloads/nsync/public
  ${VICTOR_REPO_PATH}/lib/util/source/anki
  ${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/include
  ${OPENCV_INCLUDE_PATHS}
)

# Link against OpenCV libs, but don't include OpenCV's protobuf as TF has its own
list(REMOVE_ITEM OPENCV_LIBS libprotobuf)

target_link_libraries(vic-neuralnets
  PRIVATE
  jsoncpp
  ${OPENCV_LIBS}    # NOTE: Order may matter here? (OpenCV before platform?)
  ${PLATFORM_LIBS}
)

# Can't strip because of TensorFlow's factory pattern: needed Ops will be lost
# anki_build_strip(TARGET vic-neuralnets)

