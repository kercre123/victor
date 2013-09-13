# coretech-vision/matlab/CameraCapture/CMakeLists.txt

set(MEX_LINK_LIBRARIES opencv_highgui)

build_mex(mexCameraCapture.cpp)