# coretech-vision/matlab/mex/CMakeLists.txt

set(MEX_LINK_LIBRARIES CoreTech_Common CoreTech_Common_Embedded CoreTech_Vision CoreTech_Vision_Embedded  opencv_core opencv_highgui opencv_imgproc opencv_calib3d)

file(GLOB MEX_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(MEX_FILE ${MEX_FILES})
	build_mex(${MEX_FILE})	
endforeach()


