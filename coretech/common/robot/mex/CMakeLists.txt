# coretech/common/robot/mex/CMakeLists.txt

set(MEX_LINK_LIBRARIES 
  ${OPENCV_LIBS}
  CoreTech_Common_Robot 
)

file(GLOB MEX_FILES "*.cpp")

#source_group( Coretech-Common\\\\Robot\\\\Mex FILES ${MEX_FILES} )

foreach(MEX_FILE ${MEX_FILES})
  build_mex(${MEX_FILE})  
endforeach()
