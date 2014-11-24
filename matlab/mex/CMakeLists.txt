# products-cozmo/matlab/mex/CMakeLists.txt

# SHould this check ANKICORETECH_USE_MATLAB too/instead?
if(ANKICORETECH_EMBEDDED_USE_MATLAB)

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
  
endif()
