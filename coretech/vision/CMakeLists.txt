# coretech/vision/CMakeLists.txt

project(cti_vision)

set(CORETECH_VISION_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

include(okao)

include(anki_build_cxx)

#
# cti_vision
#

anki_build_cxx_library(cti_vision ${ANKI_SRCLIST_DIR} STATIC)

set_target_properties(cti_vision 
  PROPERTIES
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
  $<$<CONFIG:Debug>:-O0>
  $<$<CONFIG:Release>:-O2>
  ${SYMBOL_VISIBILITY_FLAGS}
)

set_property(TARGET cti_vision PROPERTY CXX_STANDARD 11)

target_include_directories(cti_vision
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)

target_compile_definitions(cti_vision
  PRIVATE
  CORETECH_ENGINE
  FACE_TRACKER_FACIOMETRIC=0
  FACE_TRACKER_FACESDK=1
  FACE_TRACKER_OPENCV=2
  FACE_TRACKER_OKAO=3
  FACE_TRACKER_PROVIDER=FACE_TRACKER_OKAO
)

#
# Vision CLAD must be generated before vision library
#
add_dependencies(cti_vision cti_vision_clad)

if (MATLAB)
  add_subdirectory(robot/mex)

  add_subdirectory(matlab/mex)
  add_subdirectory(matlab/CameraCapture)

  add_subdirectory(matlab/\@VisionMarkerTrained/private)

  if(EMBEDDED_USE_OPENCV)
    add_subdirectory(tools/decisionTree)
  endif()
endif()

#
# cti_vision_test
#

if (MACOSX)

  enable_testing()

  include(gtest)

  anki_build_cxx_executable(cti_vision_test ${ANKI_SRCLIST_DIR})

  target_link_libraries(cti_vision_test
    PRIVATE
    cti_vision
    gtest
  )

  add_test(NAME cti_vision_test COMMAND cti_vision_test)

  set_tests_properties(cti_vision_test
    PROPERTIES
    ENVIRONMENT "GTEST_OUTPUT=xml:ctiVisionGoogleTest.xml"
    ENVIRONMENT "TEST_DATA_PATH=${CMAKE_CURRENT_SOURCE_DIR}"
  )

endif()

#
# Standalone Neural Nets Process
#
option(BUILD_NEURAL_NET_PROCESS "Build standalone Neural Net processor" OFF)
if (BUILD_NEURAL_NET_PROCESS)
  add_subdirectory("neuralnets")
endif()
