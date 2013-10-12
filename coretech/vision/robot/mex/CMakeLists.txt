# coretech/vision/robot/mex/CMakeLists.txt

set(MEX_LINK_LIBRARIES 
	CoreTech_Common_Robot 
	CoreTech_Vision_Robot
)

file(GLOB FILES "*.cpp")

source_group( Coretech-Vision\\\\Robot\\\\Mex FILES ${MEX_FILES} )

foreach(MEX_FILE ${MEX_FILES})
	build_mex(${MEX_FILE})	
endforeach()
