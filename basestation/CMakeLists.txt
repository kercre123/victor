# products-cozmo/basestation/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

include_directories(${COZMO_BASESTATION_INCLUDE_DIR})

if(APPLE)
  add_definitions(-DCOZMO_BASESTATION)
  add_definitions(-DCORETECH_BASESTATION)

  add_subdirectory(src)
  
  if(NOT ANKI_IOS_BUILD)
    add_subdirectory(test)
  endif()
endif()
