# coretech/common/matlab/mex/CMakeLists.txt

message(STATUS "Now in ${CMAKE_CURRENT_SOURCE_DIR}")

set(MEX_LINK_LIBRARIES 
  ${OPENCV_LIBS} 
  ctiCommonRobot 
  ctiCommon
)

#file(GLOB MEX_FILES "*.cpp")

#foreach(MEX_FILE ${MEX_FILES})
#  build_mex(${MEX_FILE})  
#endforeach()



