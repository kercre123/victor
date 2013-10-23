# coretech-vision/matlab/CameraCapture/CMakeLists.txt

set(MEX_LINK_LIBRARIES ${OPENCV_LIBS})

build_mex(mexCameraCapture.cpp)