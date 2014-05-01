# coretech-vision/matlab/CameraCapture/CMakeLists.txt

#
# mexCameraCapture
#
set(MEX_LINK_LIBRARIES ${OPENCV_LIBS})

build_mex(mexCameraCapture.cpp)


#
# mexWifiCameraCapture
#

include_directories(../../../common/tools/communication)

set(MEX_LINK_LIBRARIES
  CoreTech_Common_Robot
  CoreTech_Vision_Robot
  DebugStreamClient
  ${OPENCV_LIBS})

build_mex(mexWifiCameraCapture.cpp)

   