# coretech/common/robot/mex/CMakeLists.txt

message(STATUS "Now in ${CMAKE_CURRENT_SOURCE_DIR}")

set(MEX_LINK_LIBRARIES 
  ctiCommonRobot 
  ${OPENCV_LIBS}
)

file(GLOB MEX_FILES "*.cpp")

#source_group( Coretech-Common\\\\Robot\\\\Mex FILES ${MEX_FILES} )

foreach(MEX_FILE ${MEX_FILES})
  build_mex(${MEX_FILE})  
endforeach()
