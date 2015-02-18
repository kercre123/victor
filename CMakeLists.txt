# cozmo-game/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

# Check that path to coretech-external is defined in environment var
if (NOT DEFINED ENV{CORETECH_EXTERNAL_DIR})
  message(FATAL_ERROR "ERROR: CORETECH_EXTERNAL_DIR is undefined")
endif(NOT DEFINED ENV{CORETECH_EXTERNAL_DIR})
set(CORETECH_EXTERNAL_DIR $ENV{CORETECH_EXTERNAL_DIR})

# Set cozmo-engine dirs
set(COZMO_ENGINE_DIR ${PROJECT_SOURCE_DIR}/lib/anki/cozmo-engine)
set(CORETECH_ROOT_DIR ${COZMO_ENGINE_DIR}/coretech)

include(${CORETECH_EXTERNAL_DIR}/anki.cmake)

if(NOT DEFINED ANKI_IOS_BUILD)
  set(ANKI_IOS_BUILD 0)
endif()

if(ANKI_IOS_BUILD)
  set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "6.0")

  add_definitions(-DANKI_IOS_BUILD=1)
    
  # Matlab won't run on the phone of course, so override defaults or command-line
  # specification to use Matlab.  This also disables mex targets:
  set(USE_MATLAB 0) 
  set(EMBEDDED_USE_MATLAB 0)
  message(STATUS "Doing iOS build. Forcibly disabling Matlab use.")
  
  if(DEFINED IOS_PLATFORM AND IOS_PLATFORM STREQUAL "SIMULATOR")
    ankiProject(CozmoGame_sim)
  else()
    ankiProject(CozmoGame_iOS)
  endif()
  
else()
  
  ankiProject(CozmoGame)
    
endif(ANKI_IOS_BUILD)



set(COZMO_ENGINE_INCLUDE_DIR ${COZMO_ENGINE_DIR}/include)
file(GLOB COZMO_ENGINE_INCLUDE_FILES "${COZMO_ENGINE_INCLUDE_DIR}/anki/cozmo/shared/*.h")

set(COZMO_GAME_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

# Defining this will make Webots controller and plugins targets in cozmo-engine 
# get copied to cozmo-game/simulator
set(COZMO_GAME_SIMULATOR_DIR ${PROJECT_SOURCE_DIR}/simulator)

# This sets the root path for the PlatformPathManager when not on iOS
if (NOT SYSTEM_ROOT_PATH_DEFINED)
  add_definitions(-DSYSTEM_ROOT_PATH=${COZMO_ENGINE_DIR})
  set(SYSTEM_ROOT_PATH_DEFINED 1)
endif(NOT SYSTEM_ROOT_PATH_DEFINED)

include_directories(
  ${COZMO_ENGINE_INCLUDE_DIR}
  ${COZMO_GAME_INCLUDE_DIR}
  ${CORETECH_ROOT_DIR}/common/include
  ${CORETECH_ROOT_DIR}/messaging/include
  ${CORETECH_ROOT_DIR}/vision/include
  ${CORETECH_ROOT_DIR}/planning/include
)

# Build these whether or not ANKI_IOS_BUILD is defined:
add_subdirectory(src)
add_subdirectory(lib/anki/cozmo-engine)
# always regenerate GeneratedMessages
add_subdirectory(unity/CSharpBinding)

# Don't build these if ANKI_IOS_BUILD is defined:
if(NOT ANKI_IOS_BUILD)
  add_subdirectory(simulator)

  #if(APPLE)
  #  add_subdirectory(tools/showVoxels)
  #endif()

endif()

