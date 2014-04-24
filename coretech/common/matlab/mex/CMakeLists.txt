# coretech/common/matlab/mex/CMakeLists.txt

set(MEX_LINK_LIBRARIES 
  ${OPENCV_LIBS} 
  CoreTech_Common_Robot 
  CoreTech_Common_Basestation
)

#file(GLOB MEX_FILES "*.cpp")

#foreach(MEX_FILE ${MEX_FILES})
#  build_mex(${MEX_FILE})  
#endforeach()



