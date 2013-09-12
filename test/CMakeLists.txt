# coretech-common/test/CMakeLists.txt

# We've got got two test executables that link against the CoreTech_Common library 
# and gtest:
add_executable(coreTechCommonTests coreTechCommonTests.cpp)
target_link_libraries(coreTechCommonTests CoreTech_Common_Embedded gtest)

add_executable(run_commonTests embedded/run_commonTests.cpp embedded/commonTests.cpp)
target_link_libraries(run_commonTests CoreTech_Common_Embedded 
	gtest opencv_core eng mx opencv_highgui opencv_imgproc)



