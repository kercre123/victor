# coretech/vision/matlab/mex/CMakeLists.txt

set(MEX_LINK_LIBRARIES 
  CoreTech_Vision_Basestation
  CoreTech_Common_Basestation
  CoreTech_Vision_Robot
  CoreTech_Common_Robot
  ${OPENCV_LIBS}
)

file(GLOB MEX_FILES "*.cpp")

#source_group( Coretech-Vision\\\\Basestation\\\\Mex FILES ${MEX_FILES} )

foreach(MEX_FILE ${MEX_FILES})
  build_mex(${MEX_FILE})  
endforeach()


