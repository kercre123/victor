# products-cozmo/basestation/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

if(APPLE)
  add_definitions(-DCOZMO_BASESTATION)
  add_definitions(-DCORETECH_BASESTATION)

  add_subdirectory(src)
  
  if(NOT ANKI_IOS_BUILD)
    add_subdirectory(test)
  endif()
endif()