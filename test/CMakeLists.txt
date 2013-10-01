# products-cozmo/test/CMakeLists.txt

add_executable(cozmo_Test cozmo_Test.cpp)
target_link_libraries(cozmo_Test Cozmo gtest)