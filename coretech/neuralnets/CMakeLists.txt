# coretech/neuralnets/CMakeLists.txt

# Set this variable to the type of neural net platform to use
set(NEURAL_NET_PLATFORM TFLite)

# List of neural net options to validate the above with
set(VALID_NEURAL_NET_PLATFORMS None TensorFlow TFLite)

# Validate the setting
list(FIND VALID_NEURAL_NET_PLATFORMS ${NEURAL_NET_PLATFORM} index)
if(index EQUAL -1)
  message(FATAL_ERROR "NEURAL_NET_PLATFORM must be one of ${VALID_NEURAL_NET_PLATFORMS}")
endif()

if (NEURAL_NET_PLATFORM STREQUAL "None")
  return()
endif()

project(cti_neuralnets)

include(anki_build_cxx)

cmake_minimum_required(VERSION 3.6)

message(STATUS "Building NeuralNets process using ${NEURAL_NET_PLATFORM}")

include(anki_build_cxx)
include(anki_build_strip)
include(protobuf)

set(PLATFORM_LIBS "")
set(COMMON_LIBS
  jsoncpp
  util
  cti_common
  cti_vision
  ${OPENCV_LIBS}
)

set(PLATFORM_INCLUDES "")

set(PLATFORM_FLAGS "")
set(COMMON_FLAGS
  -Wno-undef                    # Tensorflow includes don't adhere to our lofty standards
  -Wno-shorten-64-to-32         #  "
)

set(PLATFORM_DEFINITIONS "")

set(TENSORFLOW_PATH ${ANKI_EXTERNAL_DIR}/tensorflow)

if(NEURAL_NET_PLATFORM STREQUAL "TensorFlow")

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
      ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen_VICOS/proto
      ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen_VICOS/protobuf/armeabi-v7a/include
      ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen_VICOS/downloads/eigen
      ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen_VICOS/downloads/nsync/public
      ${TENSORFLOW_PATH}
    )

    set(PLATFORM_LIBS
      victorCrashReports
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

    set(PLATFORM_FLAGS
      --std=c++11
    )
    
    set(PLATFORM_INCLUDES
      ${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac
      ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/proto
      ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/protobuf/include
      ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/downloads/eigen
      ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/downloads/nsync/public
      ${TENSORFLOW_PATH}
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

  set(PLATFORM_DEFINITIONS 
    ANKI_NEURALNETS_USE_TENSORFLOW
  )

elseif(NEURAL_NET_PLATFORM STREQUAL "TFLite")

  if (VICOS)

    set(TFLITE_DOWNLOADS_PATH ${TENSORFLOW_PATH}/tensorflow/contrib/lite/gen_VICOS/downloads)
    
    set(PLATFORM_LIBS
      victorCrashReports
      ${TENSORFLOW_PATH}/tensorflow/contrib/lite/gen_VICOS/lib/libtensorflow-lite.a
      dl
    )

  elseif (MACOSX)

    set(TFLITE_DOWNLOADS_PATH ${TENSORFLOW_PATH}/tensorflow/contrib/lite/gen_OSX/downloads)

    set(PLATFORM_LIBS
      ${TENSORFLOW_PATH}/tensorflow/contrib/lite/gen_OSX/lib/libtensorflow-lite.a
    )

  endif()

  set(PLATFORM_INCLUDES
    ${TENSORFLOW_PATH}
    ${TFLITE_DOWNLOADS_PATH}/flatbuffers/include
    ${TFLITE_DOWNLOADS_PATH}
  )

  set(PLATFORM_DEFINITIONS 
    ANKI_NEURALNETS_USE_TFLITE
    TFLITE_PROFILING_ENABLED
  )
  
else()

  message(FATAL_ERROR "NEURAL_NET_PLATFORM should be either 'TensorFlow' or 'TFLite'")

endif()

#
# cti_neuralnets library
#
anki_build_cxx_library(cti_neuralnets ${ANKI_SRCLIST_DIR} STATIC)
anki_build_target_license(cti_neuralnets "ANKI" "Apache-2.0,${CMAKE_SOURCE_DIR}/licenses/tensorflow.license")

target_compile_options(cti_neuralnets
  PRIVATE
  ${COMMON_FLAGS}
  ${PLATFORM_FLAGS}
)

target_include_directories(cti_neuralnets
  PRIVATE
  ${PLATFORM_INCLUDES}
  ${OPENCV_INCLUDE_PATHS}
)

target_compile_definitions(cti_neuralnets
  PUBLIC
  ${PLATFORM_DEFINITIONS}
)
target_link_libraries(cti_neuralnets
  PRIVATE
  ${COMMON_LIBS}
  ${PLATFORM_LIBS}
)


#
# Unit test target
#

set(TEST_TARGET_NAME cti_neuralnet_test)

if (MACOSX)

  enable_testing()

  include(gtest)

  anki_build_cxx_executable(${TEST_TARGET_NAME} ${ANKI_SRCLIST_DIR})
  anki_build_target_license(${TEST_TARGET_NAME} "ANKI")

  target_compile_definitions(${TEST_TARGET_NAME}
    PRIVATE
    TEST_DATA_PATH=${CMAKE_CURRENT_SOURCE_DIR}
    ${PLATFORM_DEFINITIONS}
  )

  target_link_libraries(${TEST_TARGET_NAME}
    PRIVATE
    ${COMMON_LIBS}
    cti_neuralnets
    gtest
  )

  add_test(NAME ${TEST_TARGET_NAME} COMMAND ${TEST_TARGET_NAME})

  set_tests_properties(${TEST_TARGET_NAME}
    PROPERTIES
    ENVIRONMENT "GTEST_OUTPUT=xml:ctiNeuralNetGoogleTest.xml"
  )

endif()
