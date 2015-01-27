# products-cozmo/robot/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

add_definitions(-DCORETECH_ROBOT)
add_definitions(-DCOZMO_ROBOT)
#add_subdirectory(supervisor/src)
#add_subdirectory(simulator)
add_subdirectory(test)
#add_subdirectory(hal)
