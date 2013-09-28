# coretech-vision/test/CMakeLists.txt

add_executable(run_visionTests 
	embedded/run_visionTests.cpp 
	embedded/visionTests.cpp)
target_link_libraries(run_visionTests 
	CoreTech_Vision_Embedded
	CoreTech_Common_Embedded
	gtest
	${OPENCV_LIBS}
	${MATLAB_ENG_LIBRARY} ${MATLAB_MX_LIBRARY} z)

add_executable(pc_visionBenchmarks 
	embedded/pc_visionBenchmarks.cpp
	embedded/visionBenchmarks.cpp)
target_link_libraries(pc_visionBenchmarks 
	CoreTech_Vision_Embedded
	CoreTech_Common_Embedded
	gtest
	${OPENCV_LIBS}
	${MATLAB_ENG_LIBRARY} ${MATLAB_MX_LIBRARY} z)


