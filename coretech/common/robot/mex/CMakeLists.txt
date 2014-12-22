# coretech/common/robot/mex/CMakeLists.txt

if(ANKICORETECH_EMBEDDED_USE_MATLAB)

  set(MEX_LINK_LIBRARIES 
    CoreTech_Common_Robot 
    ${OPENCV_LIBS}
  )

  file(GLOB MEX_FILES "*.cpp")

  #source_group( Coretech-Common\\\\Robot\\\\Mex FILES ${MEX_FILES} )

  foreach(MEX_FILE ${MEX_FILES})
    build_mex(${MEX_FILE})  
  endforeach()

endif()
