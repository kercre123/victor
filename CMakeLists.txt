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

  add_definitions(-DANKI_IOS_BUILD=1)
    
  # Matlab won't run on the phone of course, so override defaults or command-line
  # specification to use Matlab.  This also disables mex targets:
  set(USE_MATLAB 0) 
  set(EMBEDDED_USE_MATLAB 0)
  message(STATUS "Doing iOS build. Forcibly disabling Matlab use.")
  
endif(ANKI_IOS_BUILD)

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

# Build these whether or not ANKI_IOS_BUILD is defined:
add_subdirectory(basestation)
add_subdirectory(coretech)

# Don't build these if ANKI_IOS_BUILD is defined:
if(NOT ANKI_IOS_BUILD)
  add_subdirectory(robot)
  #add_subdirectory(test)
  add_subdirectory(matlab/mex)
endif()


