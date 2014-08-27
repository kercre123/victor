# coretech/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

#include(anki.cmake)
#ankiProject(CoreTech)

include_directories(
  common/include
  vision/include
  planning/include
  messaging/include
  common/tools/threads
)
 
add_subdirectory(common)
add_subdirectory(vision)
add_subdirectory(planning)
add_subdirectory(messaging)

#add_subdirectory(external)
