cmake_minimum_required(VERSION 3.6)

project(computation)

include(anki_build_cxx)
include(opencv)

# ======================================================================================================================

if (VICOS AND ANKI_BUILD_OPENCL_ROT13_EXAMPLE)

set( ROT13_SRCS
  example/rot13.cpp
)
add_executable( rot13 ${ROT13_SRCS} )
target_include_directories( rot13
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/example>
)

target_link_libraries( rot13
  OpenCL
)

endif() # VICOS and ANKI_BUILD_OPENCL_ROT13_EXAMPLE

# ======================================================================================================================

if (VICOS AND ANKI_BUILD_OPENCL_EVALUATION)

  set( OPENCL_EVALUATION_SRCS
    evaluation/src/main.cpp
    evaluation/src/Evaluator.cpp
    evaluation/src/IONMemory.cpp
    evaluation/src/Profile.cpp
    evaluation/src/Serialization.cpp
    evaluation/src/Util.cpp
  )

  add_executable( ocl_eval ${OPENCL_EVALUATION_SRCS} )

  target_include_directories( ocl_eval 
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/evaluation/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/evaluation/include>
  )

  target_link_libraries( ocl_eval
    OpenCL
    opencv_interface
    ${OPENCV_LIBS}
  )

endif() # VICOS and ANKI_BUILD_OPENCL_EVALUATION

# ======================================================================================================================

if (VICOS AND ANKI_BUILD_OPENCL_TFLITE)

  set(TENSORFLOW_PATH ${ANKI_EXTERNAL_DIR}/tensorflow)
  set(TFLITE_DOWNLOADS_PATH ${TENSORFLOW_PATH}/tensorflow/contrib/lite/gen_VICOS/downloads)

  set( OPENCL_TFLITE_SRCS
    tflite/src/main.cpp
    tflite/src/accelerator.cpp
    tflite/src/custom_layer_gpu.cpp
  )

  add_executable( ocl_tflite ${OPENCL_TFLITE_SRCS} )

  target_include_directories( ocl_tflite 
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tflite/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tflite/include>
    ${TFLITE_DOWNLOADS_PATH}/flatbuffers/include
    ${TENSORFLOW_PATH}
  )

  target_link_libraries( ocl_tflite
    PRIVATE
    OpenCL
    opencv_interface
    ${OPENCV_LIBS}
    ${TENSORFLOW_PATH}/tensorflow/contrib/lite/gen_VICOS/lib/libtensorflow-lite.a
    dl
  )

  target_compile_definitions( ocl_tflite
    PUBLIC
    TFLITE_PROFILING_ENABLED
  )

  target_compile_options( ocl_tflite
    PRIVATE
    --std=c++11
    -Wno-extern-c-compat
  )

endif() # VICOS and ANKI_BUILD_OPENCL_TFLITE

# ======================================================================================================================
