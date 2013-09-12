# coretech-common/matlab/mex/CMakeLists.txt

set(MEX_LINK_LIBRARIES opencv_highgui)

build_mex(mexEstimateHomography.cpp)


