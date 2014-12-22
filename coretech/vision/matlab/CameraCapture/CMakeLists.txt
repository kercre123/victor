# coretech-vision/matlab/CameraCapture/CMakeLists.txt

# TODO: Should this check ANKICORETECH_USE_MATLAB instead (would require changes to mexCameraCapture.cpp source)
if(ANKICORETECH_EMBEDDED_USE_MATLAB)

  #
  # mexCameraCapture
  #
  set(MEX_LINK_LIBRARIES 
    CoreTech_Common_Robot
    CoreTech_Vision_Robot
    ${OPENCV_LIBS})

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

endif()
   