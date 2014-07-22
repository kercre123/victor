# coretech/vision/matlab/@VisionMarkerTrained/private/CMakeLists.txt

# Don't use mexWrappers.cpp
set(NO_MEX_WRAPPERS 1)

build_mex(mexComputeInfoGain.cpp)  