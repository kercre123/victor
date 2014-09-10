# coretech/vision/basestation/test/CMakeLists.txt

# CoreTech_Vision Tests
add_definitions(-DCORETECH_BASESTATION)

add_executable(run_coreTechVisionTests 
  run_coreTechVisionTests.cpp 
  coreTechVisionTests.cpp)

target_link_libraries(run_coreTechVisionTests 
  CoreTech_Vision_Basestation CoreTech_Common_Basestation
  ${OPENCV_LIBS} gtest jsoncpp)
    
set_target_properties(run_coreTechVisionTests PROPERTIES
  FOLDER "Unit Tests"
)
    