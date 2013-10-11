# coretech-common/matlab/mex/CMakeLists.txt

set(CMAKE_LIBS opencv_highgui)
fix_opencv_lib_names(CMAKE_LIBS)

set(MEX_LINK_LIBRARIES ${CMAKE_LIBS})

build_mex(mexEstimateHomography.cpp)


