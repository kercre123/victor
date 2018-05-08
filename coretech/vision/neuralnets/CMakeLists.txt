# coretech/vision/neuralnets/CMakeLists.txt

#
# TensorFlow Process
#

# TODO: Use normal Cmake targets here instead of building with external scripts
# TODO: Pass in Debug vs. Release

message(STATUS "Building NeuralNets process with external build script")

set(TARGET_NAME vic-neuralnets)
set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(OUTPUT_BINARY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME})

if(VICOS)
  
  set(BUILD_CMD "./build_vicos.sh" ${REPO_ROOT} ${VICOS_SDK} ${OUTPUT_BINARY})
  
else()

  set(BUILD_CMD "./build_mac.sh" ${REPO_ROOT} ${OUTPUT_BINARY})
  
endif()

add_custom_target(${TARGET_NAME} ALL
    COMMAND ${BUILD_CMD}
    SOURCES standaloneForwardInference.cpp objectDetector_tensorflow.h objectDetector_tensorflow.cpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building standalone neuralnets process")

