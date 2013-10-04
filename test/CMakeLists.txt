# coretech-vision/test/CMakeLists.txt

add_executable(run_pc_embeddedVisionTests 
	embedded/run_pc_embeddedVisionTests.cpp 
	embedded/embeddedVisionTests.cpp)
target_link_libraries(run_pc_embeddedVisionTests 
	CoreTech_Vision_Embedded
	CoreTech_Common_Embedded
	gtest
	${OPENCV_LIBS}
	${MATLAB_ENG_LIBRARY} ${MATLAB_MX_LIBRARY} ${ZLIB_LIBRARY})

add_executable(run_pc_embeddedVisionBenchmarks 
	embedded/run_pc_embeddedVisionBenchmarks.cpp
	embedded/embeddedVisionBenchmarks.cpp)
target_link_libraries(run_pc_embeddedVisionBenchmarks 
	CoreTech_Vision_Embedded
	CoreTech_Common_Embedded
	gtest
	${OPENCV_LIBS}
	${MATLAB_ENG_LIBRARY} ${MATLAB_MX_LIBRARY} ${ZLIB_LIBRARY})


