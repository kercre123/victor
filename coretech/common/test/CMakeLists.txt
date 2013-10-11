# coretech-common/test/CMakeLists.txt

# CoreTech_Common Tests
add_executable(run_coreTechCommonTests 
	run_coreTechCommonTests.cpp 
	testMatrix.cpp
	testPoint.cpp
	testRotation.cpp)
	
target_link_libraries(run_coreTechCommonTests 
	CoreTech_Common ${OPENCV_LIBS} gtest)


# Embedded CoreTech_Common Tests
add_executable(run_pc_embeddedCommonTests 
	embedded/run_pc_embeddedCommonTests.cpp embedded/embeddedCommonTests.cpp)

target_link_libraries(run_pc_embeddedCommonTests 
	CoreTech_Common_Embedded 
	gtest ${OPENCV_LIBS} ${MATLAB_ENG_LIBRARY} ${MATLAB_MX_LIBRARY} ${ZLIB_LIBRARY})



