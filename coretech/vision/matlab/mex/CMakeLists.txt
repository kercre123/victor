# coretech/vision/matlab/mex/CMakeLists.txt

set(MEX_LINK_LIBRARIES 
	CoreTech_Common_BaseStation
	CoreTech_Vision_BaseStation
	${OPENCV_LIBS}
)

file(GLOB FILES "*.cpp")

#source_group( Coretech-Vision\\\\Basestation\\\\Mex FILES ${MEX_FILES} )

foreach(MEX_FILE ${MEX_FILES})
	build_mex(${MEX_FILE})	
endforeach()


