# coretech-vision/test/CMakeLists.txt

add_executable(run_visionTests 
	embedded/run_visionTests.cpp 
	embedded/visionTests.cpp)
target_link_libraries(run_visionTests 
	CoreTech_Vision_Embedded
	CoreTech_Common_Embedded
	gtest
	opencv_core opencv_imgproc opencv_highgui
	eng mx)

add_executable(pc_visionBenchmarks 
	embedded/pc_visionBenchmarks.cpp
	embedded/visionBenchmarks.cpp)
target_link_libraries(pc_visionBenchmarks 
	CoreTech_Vision_Embedded
	CoreTech_Common_Embedded
	gtest
	opencv_core opencv_imgproc opencv_highgui
	eng mx)


