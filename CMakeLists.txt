# cozmo-game/CMakeLists.txt

cmake_minimum_required(VERSION 3.0.0)

# http://stackoverflow.com/questions/10509380/tell-cmake-to-be-quiet
if(NOT DEFINED ANKI_OUTPUT_VERBOSE)
  function(message)
    list(GET ARGV 0 MessageType)
    #if(MessageType STREQUAL FATAL_ERROR OR
    #   MessageType STREQUAL SEND_ERROR OR
    #   MessageType STREQUAL WARNING OR
    #   MessageType STREQUAL AUTHOR_WARNING)
    #  list(REMOVE_AT ARGV 0)
    #  _message(${MessageType} "${ARGV}")
    #endif()
  endfunction()
endif()

# Check that path to coretech-external is defined in environment var
if (NOT DEFINED ENV{CORETECH_EXTERNAL_DIR})
  message(FATAL_ERROR "ERROR: CORETECH_EXTERNAL_DIR is undefined")
endif(NOT DEFINED ENV{CORETECH_EXTERNAL_DIR})
set(CORETECH_EXTERNAL_DIR $ENV{CORETECH_EXTERNAL_DIR})

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
  
  ankiProject(CozmoGame_IOS)
  
else()
  
  ankiProject(CozmoGame_MAC)
    
endif(ANKI_IOS_BUILD)


# Set cozmo-engine dirs
set(COZMO_ENGINE_DIR ${PROJECT_SOURCE_DIR}/lib/anki/cozmo-engine)
set(CORETECH_ROOT_DIR ${COZMO_ENGINE_DIR}/coretech)

include("${COZMO_ENGINE_DIR}/clad/clad.cmake")

set(ANKI_UTIL_DIR "${COZMO_ENGINE_DIR}/tools/anki-util")
# Defining this will make Webots controller and plugins targets in cozmo-engine 
# get copied to cozmo-game/simulator
set(COZMO_GAME_SIMULATOR_DIR ${PROJECT_SOURCE_DIR}/simulator)

# This sets the root path for the PlatformPathManager when not on iOS
if (NOT SYSTEM_ROOT_PATH_DEFINED)
  add_definitions(-DSYSTEM_ROOT_PATH=${COZMO_ENGINE_DIR})
  set(SYSTEM_ROOT_PATH_DEFINED 1)
endif(NOT SYSTEM_ROOT_PATH_DEFINED)

include_directories(
  ${COZMO_ENGINE_DIR}/basestation/include
  ${COZMO_ENGINE_DIR}/include
  ${PROJECT_SOURCE_DIR}/game/include
  ${CORETECH_ROOT_DIR}/common/include
  ${CORETECH_ROOT_DIR}/messaging/include
  ${CORETECH_ROOT_DIR}/vision/include
  ${CORETECH_ROOT_DIR}/planning/include
  ${CLAD_DIR}/support/cpp/include
  "${ANKI_UTIL_DIR}/source"
  "${ANKI_UTIL_DIR}/source/3rd"
  "${ANKI_UTIL_DIR}/source/3rd/jsoncpp"
  "${ANKI_UTIL_DIR}/source/anki"
  "${ANKI_UTIL_DIR}/libs/packaged/include"
)

set(CMAKE_CXX_FLAGS "-Wno-unused-function -Wno-overloaded-virtual -Wno-deprecated-declarations -Wno-unused-variable")

# Build these whether or not ANKI_IOS_BUILD is defined:
add_subdirectory(clad)
add_subdirectory(lib/anki/cozmo-engine)
add_subdirectory(game)
add_subdirectory(unity/CSharpBinding)

if(NOT ANKI_IOS_BUILD)
  add_subdirectory(simulator)
endif()
