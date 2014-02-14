# products-cozmo/test/CMakeLists.txt

if(NOT WIN32)
  add_executable(cozmo_Test cozmo_Test.cpp)
  target_link_libraries(cozmo_Test Cozmo gtest)
endif()
