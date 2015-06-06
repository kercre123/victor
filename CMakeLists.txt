# products-cozmo/CMakeLists.txt

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
  
  ankiProject(CozmoEngine_IOS)
  
else()
  
  ankiProject(CozmoEngine_MAC)
    
endif(ANKI_IOS_BUILD)


include("${CMAKE_CURRENT_SOURCE_DIR}/clad/clad.cmake")

set(ANKI_UTIL_DIR "${PROJECT_SOURCE_DIR}/tools/anki-util")
set(COZMO_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(COZMO_BASESTATION_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/basestation/include)
file(GLOB COZMO_INCLUDE_FILES "${COZMO_INCLUDE_DIR}/anki/cozmo/shared/*.h")

# This sets the root path for the PlatformPathManager when not on iOS
if (NOT SYSTEM_ROOT_PATH_DEFINED)
  add_definitions(-DSYSTEM_ROOT_PATH=${PROJECT_SOURCE_DIR})
  set(SYSTEM_ROOT_PATH_DEFINED 1)
endif(NOT SYSTEM_ROOT_PATH_DEFINED)

include_directories(
  ${COZMO_INCLUDE_DIR}
  coretech/common/include
  coretech/messaging/include
  coretech/vision/include
  coretech/planning/include
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
add_subdirectory(basestation)
add_subdirectory(coretech)

# anki-util is a submodule we don't want to stick cmake in
add_subdirectory(tools)


# Don't build these if ANKI_IOS_BUILD is defined:
if(NOT ANKI_IOS_BUILD)
  add_subdirectory(robot)
  add_subdirectory(simulator)
  #add_subdirectory(test)
  add_subdirectory(matlab/mex)

  if(APPLE)
    add_subdirectory(tools/showVoxels)
    
    if(BUILD_RECOGNIZE_FACES)
      add_subdirectory(tools/recognizeFaces)
    endif()
  endif()
  
  add_subdirectory(tools/captureImages)

endif()


