# products-cozmo/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

include(coretech/anki.cmake)

ankiProject(Cozmo)

set(COZMO_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
file(GLOB COZMO_INCLUDE_FILES "${COZMO_INCLUDE_DIR}/anki/cozmo/shared/*.h")

# This sets the root path for the PlatformPathManager when not on iOS
add_definitions(-DSYSTEM_ROOT_PATH=${PROJECT_SOURCE_DIR})

include_directories(
  ${COZMO_INCLUDE_DIR}
  coretech/common/include
  coretech/messaging/include
  coretech/vision/include
  coretech/planning/include
)

add_subdirectory(basestation)
add_subdirectory(robot)

#add_subdirectory(test)
add_subdirectory(matlab/mex)

add_subdirectory(coretech)

if(APPLE)
  add_subdirectory(tools/showVoxels)
endif()