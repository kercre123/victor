# cozmo-engine/matlab/mex/CMakeLists.txt

message(STATUS "Now in ${CMAKE_CURRENT_SOURCE_DIR}")

set(MEX_LINK_LIBRARIES 
  #CoreTech_Common_Basestation
  #CoreTech_Vision_Basestation
  ctiVisionRobot
  ctiCommonRobot
  ${OPENCV_LIBS}
)

add_definitions(-DOPENCV_ROOT_PATH=${EXTERNAL_DIR}/${OPENCV_DIR})

build_mex(mexFaceDetect.cpp)
build_mex(mexDrawSystemTestResults.cpp)
#build_mex(mexTest.cpp)
