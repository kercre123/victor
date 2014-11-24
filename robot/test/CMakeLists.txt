# coretech/common/robot/test/CMakeLists.txt

# Robot CoreTech_Common Tests

add_executable(run_pc_embeddedTests
  embeddedTests.h
  embeddedTests.cpp
  run_pc_embeddedTests.cpp  
  embeddedCommonTests.cpp
  embeddedVisionTests.cpp)

target_link_libraries(run_pc_embeddedTests
  CoreTech_Vision_Robot
  CoreTech_Common_Robot
  ${ALL_EMBEDDED_LIBRARIES}
  ${MATLAB_MEX_LIBRARY})

set_target_properties(run_pc_embeddedTests PROPERTIES
  FOLDER "Unit Tests"
)
