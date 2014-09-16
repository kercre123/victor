# coretech/planning/basestation/test/CMakeLists.txt

# CoreTech_Planner standalone planner binary
add_executable(run_coreTechPlanningStandalone
  run_coreTechPlanningStandalone.cpp)

target_link_libraries(run_coreTechPlanningStandalone
        CoreTech_Planning_Basestation
        CoreTech_Common_Basestation
        jsoncpp ${OPENCV_LIBS})
          
set_property(TARGET run_coreTechPlanningStandalone
        PROPERTY FOLDER "Unit Tests"
)

include_directories(../src)

add_executable(run_coreTechPlanningTests
        run_coreTechPlanningTests.cpp
        testEnvironment.cpp
        testPlanner.cpp
)

target_link_libraries(run_coreTechPlanningTests
        CoreTech_Planning_Basestation
        CoreTech_Common_Basestation
        jsoncpp
        gtest ${OPENCV_LIBS})
          
set_target_properties(run_coreTechPlanningTests PROPERTIES
       FOLDER "Unit Tests"
)
