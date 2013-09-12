# anki.cmake
#
#  This file sets up some common CMake structure and names for use by
#  coretech-* and products-* libraries.
#
macro(ankiProject PROJECT_NAME)

# Suppress warning message about relative vs. absolute paths:
cmake_policy(SET CMP0015 NEW)

set(OPENCV_DIR opencv-2.4.6.1)
set(GTEST_DIR gtest-1.7.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

if(WIN32)
	set(LIBRARY_BITS 32)
elseif(UNIX)
	set(LIBRARY_BITS 64)
else()
	message(FATAL_ERROR "Not sure what to set LIBRARY_BITS to.")
endif()

#include(FindMatlab)
set(MATLAB_INCLUDE_DIR /Applications/MATLAB_R2013a.app/extern/include)
set(MATLAB_LIB_DIR /Applications/MATLAB_R2013a.app/bin/maci64)
message(STATUS "Using Matlab include dir ${MATLAB_INCLUDE_DIR}")

project(${PROJECT_NAME})

# Stuff after this requires project() to have been called

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

# External libraries
# We assume that coretech-external lives alongside each project
# for which we use this anki.cmake file.
set(EXTERNAL_DIR ${PROJECT_SOURCE_DIR}/../coretech-external)
set(OPENCV_MODULES_DIR ${EXTERNAL_DIR}/${OPENCV_DIR}/modules)
include_directories(
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/../coretech-common/include 
	${EXTERNAL_DIR}/${OPENCV_DIR}/include 
	${EXTERNAL_DIR}/${GTEST_DIR}/include
	${MATLAB_INCLUDE_DIR})

# Add the include directory for each OpenCV module:
file(GLOB OPENCV_MODULES RELATIVE ${OPENCV_MODULES_DIR} ${OPENCV_MODULES_DIR}/*)
foreach(OPENCV_MODULE ${OPENCV_MODULES})
	if(IS_DIRECTORY ${OPENCV_MODULES_DIR}/${OPENCV_MODULE})
		include_directories(${OPENCV_MODULES_DIR}/${OPENCV_MODULE}/include)
	endif()
endforeach()

if(CMAKE_GENERATOR MATCHES "Unix Makefiles")
	message("Appending ${CMAKE_BUILD_TYPE} to output directories since Unix Makefiles are being used.")
	set(BUILD_TYPE_DIR ${CMAKE_BUILD_TYPE})
else()
	set(BUILD_TYPE_DIR ./)	
endif()

# Store our libraries in, e.g., coretech-vision/build/lib/<Debug>
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib/${BUILD_TYPE_DIR})

# Store our executables such as tests in, e.g., coretech-vision/build/bin
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin/${BUILD_TYPE_DIR})

link_directories(
	${PROJECT_SOURCE_DIR}/build/lib/${BUILD_TYPE_DIR}
	${PROJECT_SOURCE_DIR}/../coretech-common/build/lib/${BUILD_TYPE_DIR} 
	${EXTERNAL_DIR}/build/${CMAKE_GENERATOR}/${OPENCV_DIR}/lib/${CMAKE_CFG_INTDIR}
	${EXTERNAL_DIR}/build/${CMAKE_GENERATOR}/${GTEST_DIR}
	${MATLAB_LIB_DIR})

endmacro(ankiProject)
