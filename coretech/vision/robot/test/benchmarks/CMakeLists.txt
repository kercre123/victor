
add_executable(run_pc_embeddedVisionBenchmarks 
	run_pc_embeddedVisionBenchmarks.cpp
	embeddedVisionBenchmarks.cpp
)

target_link_libraries(run_pc_embeddedVisionBenchmarks 
	CoreTech_Vision_Robot
	CoreTech_Common_Robot
	gtest
	${OPENCV_LIBS}
	${MATLAB_ENG_LIBRARY} ${MATLAB_MX_LIBRARY} ${ZLIB_LIBRARY})


