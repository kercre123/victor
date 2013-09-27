# coretech-vision/matlab/mex/CMakeLists.txt

if(WIN32)
	set(MEX_LINK_LIBRARIES CoreTech_Common_Embedded CoreTech_Vision_Embedded  opencv_core246d opencv_highgui246d opencv_imgproc246d opencv_calib3d246d)
else()
	set(MEX_LINK_LIBRARIES CoreTech_Common CoreTech_Common_Embedded CoreTech_Vision
 CoreTech_Vision_Embedded  opencv_core opencv_highgui opencv_imgproc opencv_calib3d)
endif(WIN32)
 
file(GLOB MEX_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(MEX_FILE ${MEX_FILES})
	build_mex(${MEX_FILE})	
endforeach()


