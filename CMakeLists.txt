# products-cozmo/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

include(coretech/anki.cmake)

if(NOT DEFINED ANKI_IOS_BUILD)
  set(ANKI_IOS_BUILD 0)
endif()

if(ANKI_IOS_BUILD)
  set(DEVROOT "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer")
  set(CMAKE_OSX_SYSROOT "${DEVROOT}/SDKs/iPhoneOS.sdk")
  
  set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_UNIVERSAL_IPHONE_OS)")
  
  set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos;-iphonesimulator") 

  set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "6.0")

  add_definitions(-DANKI_IOS_BUILD=1)
    
  # Matlab won't run on the phone of course, so override defaults or command-line
  # specification to use Matlab.  This also disables mex targets:
  set(USE_MATLAB 0) 
  set(EMBEDDED_USE_MATLAB 0)
  message(STATUS "Doing iOS build. Forcibly disabling Matlab use.")
  
  ankiProject(CozmoEngine_iOS)
  
  # TODO: this is a weird place for this, but until we have an external iOS build, it's the only
  #       thing we need from coretech-external for the iOS build, so i'm just doing it here for simplicity
  add_subdirectory(${PROJECT_SOURCE_DIR}/../coretech-external/jsoncpp "${CMAKE_CURRENT_BINARY_DIR}/jsoncpp")
  
else()
  
  ankiProject(CozmoEngine)
    
endif(ANKI_IOS_BUILD)



set(COZMO_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
file(GLOB COZMO_INCLUDE_FILES "${COZMO_INCLUDE_DIR}/anki/cozmo/shared/*.h")

# NOTE: This will eventually get pulled out of this repo
set(COZMO_GAME_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/game/include)

# This sets the root path for the PlatformPathManager when not on iOS
add_definitions(-DSYSTEM_ROOT_PATH=${PROJECT_SOURCE_DIR})

include_directories(
  ${COZMO_INCLUDE_DIR}
  ${COZMO_GAME_INCLUDE_DIR}
  coretech/common/include
  coretech/messaging/include
  coretech/vision/include
  coretech/planning/include
)

# Build these whether or not ANKI_IOS_BUILD is defined:
add_subdirectory(basestation)
add_subdirectory(coretech)

# TODO: This will be pulled out into its own repo
add_subdirectory(game)

# Don't build these if ANKI_IOS_BUILD is defined:
if(NOT ANKI_IOS_BUILD)
  add_subdirectory(robot)
  #add_subdirectory(test)
  add_subdirectory(matlab/mex)

  if(APPLE)
    add_subdirectory(tools/showVoxels)
  endif()

endif()


