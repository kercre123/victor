# coretech/common/matlab/mex/CMakeLists.txt

set(EXTRA_OPENCV_LIBS opencv_highgui)
fix_opencv_lib_names(EXTRA_OPENCV_LIBS)

set(MEX_LINK_LIBRARIES ${EXTRA_OPENCV_LIBS} ${OPENCV_LIBS})

build_mex(mexEstimateHomography.cpp)


