# coretech-vision/matlab/CameraCapture/CMakeLists.txt

message(STATUS "Now in ${CMAKE_CURRENT_SOURCE_DIR}")

  #
  # mexCameraCapture
  #
  set(MEX_LINK_LIBRARIES 
    ctiCommonRobot
    ctiVisionRobot
    ${OPENCV_LIBS})

  build_mex(mexCameraCapture.cpp)


  #
  # mexWifiCameraCapture
  #

#  include_directories(../../../common/tools/communication)
#
#  set(MEX_LINK_LIBRARIES
#    ctiCommonRobot
#    ctiVisionRobot
#    DebugStreamClient
#    ${OPENCV_LIBS})
#
#  build_mex(mexWifiCameraCapture.cpp)
   