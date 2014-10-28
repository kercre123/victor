# coretech/common/basestation/test/CMakeLists.txt

# CoreTech_Common Tests
        
add_definitions(-DCORETECH_BASESTATION)

add_executable(run_coreTechCommonTests 
  run_coreTechCommonTests.cpp 
  testMatrix.cpp
  testPoint.cpp
  testQuad.cpp
  testPolygon.cpp
  testRotation.cpp
  testRotatedRectangle.cpp
  testConfigTreeJsonCpp.cpp)

target_link_libraries(run_coreTechCommonTests 
  CoreTech_Common_Basestation ${OPENCV_LIBS} gtest jsoncpp)
    
set_target_properties(run_coreTechCommonTests PROPERTIES
  FOLDER "Unit Tests"
)
  
