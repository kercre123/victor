# coretech/vision/CMakeLists.txt

set(CORETECH_VISION_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/anki/vision)

file(GLOB CORETECH_VISION_INCLUDE_FILES "${CORETECH_VISION_INCLUDE_DIR}/*.h")

if(APPLE)
  add_subdirectory(basestation/src)
  
  if(NOT ANKI_IOS_BUILD)
    add_subdirectory(basestation/test)
  endif()
endif()

# TODO: this can probably be skipped for iOS builds once we aren't using embedded code on Basestation for vision
add_subdirectory(robot/src)

if(NOT ANKI_IOS_BUILD)
  add_subdirectory(robot/mex)

  add_subdirectory(matlab/mex)
  add_subdirectory(matlab/CameraCapture)

  add_subdirectory(matlab/\@VisionMarkerTrained/private)

  if(EMBEDDED_USE_OPENCV)
    add_subdirectory(tools/decisionTree)
  endif()
endif()
