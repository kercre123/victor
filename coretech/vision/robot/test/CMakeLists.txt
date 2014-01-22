# coretech/vision/robot/test/CMakeLists.txt

add_executable(run_pc_embeddedVisionTests
	run_pc_embeddedVisionTests.cpp
	embeddedVisionTests.cpp)

target_link_libraries(run_pc_embeddedVisionTests
	CoreTech_Vision_Robot
	CoreTech_Common_Robot
	${ALL_EMBEDDED_LIBRARIES})
