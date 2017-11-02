# coretech/vision/CMakeLists.txt

project(cti_vision)

set(CORETECH_VISION_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/anki/vision)

include(okao)

include(anki_build_cxx)

# add_library()
anki_build_cxx_library(cti_vision ${ANKI_SRCLIST_DIR} STATIC)

set_target_properties(cti_vision PROPERTIES
  FOLDER "CoreTech"#/Basestation"
)

target_link_libraries(cti_vision
  cti_common
  jsoncpp
  util
  ${OPENCV_LIBS}
  ${OKAO_LIBS}
)

target_compile_options(cti_vision
  PRIVATE
  ${SYMBOL_VISIBILITY_FLAGS}
)

set_property(TARGET cti_vision PROPERTY CXX_STANDARD 11)

target_include_directories(cti_vision
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/basestation/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>  # For access to engine/vision/nearestNeighborLibraryData.h 

  # TODO: Somehow use the INTERFACE_INCLUDE_DIRECTORIES from tensorflow.cmake?
  ${TENSORFLOW_INCLUDE_PATH}/protobuf
  ${TENSORFLOW_INCLUDE_PATH}/tensorflow
  ${TENSORFLOW_INCLUDE_PATH}/proto
)

target_compile_definitions(cti_vision
  PRIVATE
  CORETECH_BASESTATION
  FACE_TRACKER_FACIOMETRIC=0
  FACE_TRACKER_FACESDK=1
  FACE_TRACKER_OPENCV=2
  FACE_TRACKER_OKAO=3
  FACE_TRACKER_PROVIDER=FACE_TRACKER_OKAO
  PROTOBUF_INLINE_NOT_IN_HEADERS=1 # To make Tensorflow happy
)

if(MATLAB)
  add_subdirectory(robot/mex)

  add_subdirectory(matlab/mex)
  add_subdirectory(matlab/CameraCapture)

  add_subdirectory(matlab/\@VisionMarkerTrained/private)

  if(EMBEDDED_USE_OPENCV)
    add_subdirectory(tools/decisionTree)
  endif()
endif()

if (MACOSX)

  enable_testing()

  include(gtest)

  anki_build_cxx_executable(cti_vision_basestation_test ${ANKI_SRCLIST_DIR})

  target_link_libraries(cti_vision_basestation_test
    PRIVATE
    cti_vision
    gtest
    ${TENSORFLOW_LIBS}
  )

  target_compile_definitions(cti_vision_basestation_test PRIVATE                                                       
    TEST_DATA_PATH=${CMAKE_CURRENT_SOURCE_DIR}
  )

  add_test(NAME cti_vision_basestation_test COMMAND cti_vision_basestation_test)

endif()

