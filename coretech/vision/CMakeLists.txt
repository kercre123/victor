# coretech/vision/CMakeLists.txt

set(CORETECH_VISION_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/anki/vision)

file(GLOB CORETECH_VISION_INCLUDE_FILES "${CORETECH_VISION_INCLUDE_DIR}/*.h")

add_subdirectory(basestation/src)
add_subdirectory(basestation/test)

add_subdirectory(robot/src)
add_subdirectory(robot/mex)

add_subdirectory(matlab/mex)
add_subdirectory(matlab/CameraCapture)

add_subdirectory(matlab/\@VisionMarkerTrained/private)

add_subdirectory(tools/decisionTree)