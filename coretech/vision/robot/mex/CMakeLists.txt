# coretech/vision/robot/mex/CMakeLists.txt

if(ANKICORETECH_EMBEDDED_USE_MATLAB)

  set(MEX_LINK_LIBRARIES 
    ${OPENCV_LIBS}
    CoreTech_Common_Robot 
    CoreTech_Vision_Robot
  )

  file(GLOB MEX_FILES "*.cpp")

  #source_group( Coretech-Vision\\\\Robot\\\\Mex FILES ${MEX_FILES} )

  foreach(MEX_FILE ${MEX_FILES})
    build_mex(${MEX_FILE})  
  endforeach()

endif()
