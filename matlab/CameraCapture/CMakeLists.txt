# coretech-vision/matlab/CameraCapture/CMakeLists.txt

set(MEX_LINK_LIBRARIES opencv_highgui opencv_imgproc)
fix_opencv_lib_names(MEX_LINK_LIBRARIES)

build_mex(mexCameraCapture.cpp)