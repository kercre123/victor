# coretech/vision/matlab/mex/CMakeLists.txt

message(STATUS "Now in ${CMAKE_CURRENT_SOURCE_DIR}")

set(MEX_LINK_LIBRARIES 
  ctiVision
  ctiCommon
  ctiVisionRobot
  ctiCommonRobot
  ${OPENCV_LIBS}
  matconvnet
  mwblas
)

file(GLOB MEX_FILES "*.cpp")

#source_group( Coretech-Vision\\\\Basestation\\\\Mex FILES ${MEX_FILES} )

foreach(MEX_FILE ${MEX_FILES})
  build_mex(${MEX_FILE})  
endforeach()


