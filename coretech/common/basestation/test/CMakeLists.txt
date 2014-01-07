# coretech/common/basestation/test/CMakeLists.txt

# CoreTech_Common Tests
add_executable(run_coreTechCommonTests 
	run_coreTechCommonTests.cpp 
	testMatrix.cpp
	testPoint.cpp
	testRotation.cpp
        testConfigTreeJsonCpp.cpp)
	
target_link_libraries(run_coreTechCommonTests 
	CoreTech_Common_Basestation ${OPENCV_LIBS} gtest jsoncpp)
