# coretech/planning/basestation/test/CMakeLists.txt

# CoreTech_Planner standalone planner binary
if(NOT WIN32)
  add_executable(run_coreTechPlanningStandalone
    run_coreTechPlanningStandalone.cpp)

  target_link_libraries(run_coreTechPlanningStandalone
    CoreTech_Planning_Basestation
          CoreTech_Common_Basestation
          jsoncpp)

  include_directories(../src)

  add_executable(run_coreTechPlanningTests
          run_coreTechPlanningTests.cpp
          testRectangle.cpp
          testEnvironment.cpp)

  target_link_libraries(run_coreTechPlanningTests
          CoreTech_Planning_Basestation
          CoreTech_Common_Basestation
          jsoncpp
          gtest)
endif()
