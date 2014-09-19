# products-cozmo/matlab/mex/CMakeLists.txt

set(MEX_LINK_LIBRARIES 
  #CoreTech_Common_Basestation
  #CoreTech_Vision_Basestation
  CoreTech_Vision_Robot
  CoreTech_Common_Robot
  ${OPENCV_LIBS}
)

add_definitions(-DOPENCV_ROOT_PATH=${EXTERNAL_DIR}/${OPENCV_DIR})

build_mex(mexFaceDetect.cpp)
build_mex(mexDrawSystemTestResults.cpp)
#build_mex(mexTest.cpp)
