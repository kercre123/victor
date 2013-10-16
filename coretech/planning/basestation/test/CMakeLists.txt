# coretech/planning/basestation/test/CMakeLists.txt

# CoreTech_Planner standalone planner binary
add_executable(run_coreTechPlanningStandalone
	run_coreTechPlanningStandalone.cpp)

target_link_libraries(run_coreTechPlanningStandalone
	CoreTech_Planning_Basestation)
