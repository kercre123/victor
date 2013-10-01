# coretech-common/test/CMakeLists.txt

# CoreTech_Common Tests
add_executable(run_coreTechCommonTests 
	run_coreTechCommonTests.cpp 
	testMatrix.cpp
	testRotation.cpp)
	
target_link_libraries(run_coreTechCommonTests 
	CoreTech_Common ${OPENCV_LIBS} gtest)


# Embedded CoreTech_Common Tests
add_executable(run_pcEmbeddedCommonTests 
	embedded/run_pcEmbeddedCommonTests.cpp embedded/embeddedCommonTests.cpp)

target_link_libraries(run_pcEmbeddedCommonTests 
	CoreTech_Common_Embedded 
	gtest ${OPENCV_LIBS} ${MATLAB_ENG_LIBRARY} ${MATLAB_MX_LIBRARY} z)


