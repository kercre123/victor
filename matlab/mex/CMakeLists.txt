# coretech-common/matlab/mex/CMakeLists.txt

if(WIN32)
	set(MEX_LINK_LIBRARIES opencv_highgui246d)
else()
	set(MEX_LINK_LIBRARIES opencv_highgui)
endif(WIN32)

build_mex(mexEstimateHomography.cpp)


