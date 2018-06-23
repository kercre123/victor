# coretech/vision/neuralnets/CMakeLists.txt

cmake_minimum_required(VERSION 3.6)

message(STATUS "Building NeuralNets process")

include(anki_build_cxx)
include(anki_build_strip)
include(protobuf)

set(TENSORFLOW_PATH  ${ANKI_EXTERNAL_DIR}/tensorflow)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_FLAGS "")
set(TARGET_NAME "")

if (VICOS)

  set(TARGET_NAME vic-neuralnets)

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
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen_VICOS/proto
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen_VICOS/protobuf/armeabi-v7a/include
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen_VICOS/downloads/eigen
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen_VICOS/downloads/nsync/public
  )

  set(PLATFORM_LIBS
    -Wl,--whole-archive 
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen_VICOS/lib/libtensorflow-core.a
    -Wl,--no-whole-archive
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen_VICOS/protobuf/armeabi-v7a/lib/libprotobuf.a
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen_VICOS/downloads/nsync/builds/default.vicos.c++11/libnsync.a
    c++   # NOTE: VicOS uses libc++, so TensorFlow must be built to match
    dl
    log
    z
  )

elseif (MACOSX)
  
  # Note that for Mac builds we are really building a Webots controller 
  # but since it shares the same main.cpp file as the vic-neuralnets process
  # we're doing it here instead of in simulator/CMakeLists.txt with the other
  # Webots controllers

  set(TARGET_NAME webotsCtrlNeuralNets) 

  include(webots)

  set(PLATFORM_FLAGS
    --std=c++11 
    -DSIMULATOR
  )
  
  set(PLATFORM_INCLUDES
    ${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/proto
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/protobuf/include
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/downloads/eigen
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/downloads/nsync/public
  )

  set(PLATFORM_LIBS
    -Wl,-force_load
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/lib/libtensorflow-core.a
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/protobuf/lib/libprotobuf.a
    ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/downloads/nsync/builds/default.macos.c++11/libnsync.a
    ${WEBOTS_LIBS}
    stdc++
    m
  )

endif()

anki_build_cxx_executable(${TARGET_NAME} ${ANKI_SRCLIST_DIR})
anki_build_target_license(${TARGET_NAME} "ANKI")

target_compile_definitions(${TARGET_NAME}
  PRIVATE
  VIC_NEURALNETS_USE_TENSORFLOW
)

target_compile_options(${TARGET_NAME}
  PRIVATE 
  -Wno-undef            # Tensorflow includes don't adhere to our lofty standards
  -Wno-shorten-64-to-32 #  "
  ${PLATFORM_FLAGS}
)

target_include_directories(${TARGET_NAME}
  PRIVATE
  ${PLATFORM_INCLUDES}
  ${TENSORFLOW_PATH}
  ${OPENCV_INCLUDE_PATHS}
)

# Link against OpenCV libs, but don't include OpenCV's protobuf as TF has its own
# NOTE: This disgusting hack should be temporary: either protobuf disappears as part of 
#       OPENCV_LIBS (if only needed by the DNN module, which we hope to remove) or when
#       we switch to TensorFlow Lite, which uses Flatbuffers.
set(OPENCV_LIBS_NO_PROTOBUF ${OPENCV_LIBS}) 
list(REMOVE_ITEM OPENCV_LIBS_NO_PROTOBUF protobuf)

target_link_libraries(${TARGET_NAME}
  PRIVATE
  jsoncpp
  util
  cti_common
  cti_vision_clad               # Just vision CLAD, not the whole enchilada
  ${OPENCV_LIBS_NO_PROTOBUF}    # NOTE: Order may matter here? (OpenCV before platform?)
  ${PLATFORM_LIBS}
)

anki_build_strip(TARGET ${TARGET_NAME})

