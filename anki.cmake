# anki.cmake
#
#  This file sets up some common CMake structure and names and helpers
#  for use by coretech-* and products-* libraries.
#
# Anki, Inc.
# Andrew Stein
# Created: 9/12/2013

# 
# Use this effectively as a replacement for cmake's "project" command
# (which this macro calls for you)
#
macro(ankiProject PROJECT_NAME)

# Suppress warning message about relative vs. absolute paths:
cmake_policy(SET CMP0015 NEW)

# So long as we're using full version names in our external libraries,
# lets not have to hardcode them all over the place:
set(OPENCV_DIR opencv-2.4.6.1)
set(GTEST_DIR gtest-1.7.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

#include(FindMatlab) # This Find script doesn't seem to work on Mac!
if(NOT DEFINED MATLAB_ROOT_DIR)
	# TODO: default windows location!
	set(MATLAB_ROOT_DIR /Applications/MATLAB_R2013a.app)
endif(NOT DEFINED MATLAB_ROOT_DIR)
set(MATLAB_INCLUDE_DIR ${MATLAB_ROOT_DIR}/extern/include)
set(MATLAB_LIB_DIR ${MATLAB_ROOT_DIR}/bin/maci64)
set(MEX_COMPILER ${MATLAB_ROOT_DIR}/bin/mex)
set(MATLAB_MEXEXT mexmaci64)
message(STATUS "Using Matlab include dir ${MATLAB_INCLUDE_DIR}")

project(${PROJECT_NAME})

# Stuff after this requires project() to have been called (I think)

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
	${MATLAB_INCLUDE_DIR}
	${PROJECT_SOURCE_DIR}/../coretech-common/matlab/mex
)

# Add the include directory for each OpenCV module:
file(GLOB OPENCV_MODULES RELATIVE ${OPENCV_MODULES_DIR} ${OPENCV_MODULES_DIR}/*)
foreach(OPENCV_MODULE ${OPENCV_MODULES})
	if(IS_DIRECTORY ${OPENCV_MODULES_DIR}/${OPENCV_MODULE})
		include_directories(${OPENCV_MODULES_DIR}/${OPENCV_MODULE}/include)
	endif()
endforeach()

# If we don't do this, we won't get our binaries in Debug/Release
# subdirectories.  That's not necessarily a bad thing, but it's the 
# way Xcode (and MSVC?) do it, and it seems easier to make the Makefile
# generator mimic that behavior as follows than to keep those IDEs from 
# doing what they wanna do.
if(CMAKE_GENERATOR MATCHES "Unix Makefiles")
	message("Appending ${CMAKE_BUILD_TYPE} to output directories since Unix Makefiles are being used.")
	set(BUILD_TYPE_DIR ${CMAKE_BUILD_TYPE})
else()
	set(BUILD_TYPE_DIR ./)	
endif()

# Store our libraries in, e.g., coretech-vision/build/lib/Debug
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib/${BUILD_TYPE_DIR})

# Store our executables such as tests in, e.g., coretech-vision/build/bin/Debug
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin/${BUILD_TYPE_DIR})

link_directories(
	${PROJECT_SOURCE_DIR}/build/lib/${BUILD_TYPE_DIR}
	${PROJECT_SOURCE_DIR}/../coretech-common/build/lib/${BUILD_TYPE_DIR} 
	${EXTERNAL_DIR}/build/${CMAKE_GENERATOR}/${OPENCV_DIR}/lib/${CMAKE_CFG_INTDIR}
	${EXTERNAL_DIR}/build/${CMAKE_GENERATOR}/${GTEST_DIR}
	${MATLAB_LIB_DIR}
)

endmacro(ankiProject)


#
# A helper macro for building mex files and linking them against our
# "standard" set of libraries, namely:
#	 opencv_core, CoreTech_Common, and CoreTech_Common_Embedded.
#
# You can link against others by setting the variable "MEX_LINK_LIBRARIES" 
# before calling this macro, e.g.:
#	set(MEX_LINK_LIBRARIES opencv_highgui CoreTech_Vision)
#
# I'm not 100% sure this totally perfect, but so far it seems to be working.
# Tweaking and improvements are welcome.  See also:
#   http://www.cmake.org/Wiki/CMake:MatlabMex
#
macro(build_mex MEX_FILE)

	#message(STATUS "Adding mex file ${MEX_FILE}, linked against ${MEX_LINK_LIBRARIES}")

	set(CC ${MEX_COMPILER})
	set(CXX ${MEX_COMPILER})
	unset(CMAKE_CXX_FLAGS)
	unset(CMAKE_C_FLAGS)
	
	# Store our libraries in, e.g., coretech-vision/build/mex
	set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/mex)

	get_filename_component(OUTPUT_NAME ${MEX_FILE} NAME_WE)
	
	add_library(${OUTPUT_NAME} SHARED 
		${MEX_FILE} 
		${PROJECT_SOURCE_DIR}/../coretech-common/matlab/mex/mexWrappers.cpp
		${PROJECT_SOURCE_DIR}/../coretech-common/matlab/mex/mexEmbeddedWrappers.cpp
		${PROJECT_SOURCE_DIR}/../coretech-common/matlab/mex/mexFunction.def
	)

	target_link_libraries(${OUTPUT_NAME} mex mx mat opencv_core CoreTech_Common CoreTech_Common_Embedded)

	if(DEFINED MEX_LINK_LIBRARIES)
		foreach(LIB ${MEX_LINK_LIBRARIES})
			  target_link_libraries(${OUTPUT_NAME} ${LIB})		
		endforeach()
	endif()

	set_target_properties(${OUTPUT_NAME} PROPERTIES
		PREFIX ""
		SUFFIX ".${MATLAB_MEXEXT}"
	)

endmacro(build_mex)