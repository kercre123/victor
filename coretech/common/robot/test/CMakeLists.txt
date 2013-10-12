# coretech/common/robot/test/CMakeLists.txt

# Robot CoreTech_Common Tests

add_executable(run_pc_embeddedCommonTests 
	run_pc_embeddedCommonTests.cpp 
	embeddedCommonTests.cpp)

target_link_libraries(run_pc_embeddedCommonTests 
	CoreTech_Common_Robot 
	gtest ${OPENCV_LIBS} 
	${MATLAB_ENG_LIBRARY} ${MATLAB_MX_LIBRARY} ${ZLIB_LIBRARY})



