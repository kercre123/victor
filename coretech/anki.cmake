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

if(NOT PROJECT_NAME MATCHES "CoreTech")
  set(CORETECH_ROOT_DIR ${PROJECT_SOURCE_DIR}/coretech)
endif(NOT PROJECT_NAME MATCHES "CoreTech")

# Suppress warning message about relative vs. absolute paths:
cmake_policy(SET CMP0015 NEW)

# If user provided flags from the command line for using Matlab/OpenCV/Gtest,
# use those.  Otherwise, set them all to the defaults defined below
set(DEFAULT_USE_MATLAB 1)
set(DEFAULT_EMBEDDED_USE_MATLAB 1)
set(DEFAULT_USE_OPENCV 1)
set(DEFAULT_EMBEDDED_USE_OPENCV 1)
set(DEFAULT_USE_GTEST 1)
set(DEFAULT_EMBEDDED_USE_GTEST 1)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(LINUX 1)
else()
  set(LINUX 0)
endif()

set(PKG_OPTIONS
  USE_MATLAB USE_GTEST USE_OPENCV
  EMBEDDED_USE_MATLAB EMBEDDED_USE_GTEST EMBEDDED_USE_OPENCV
)

foreach(PKG ${PKG_OPTIONS})
  if(DEFINED ${PKG})
      # message(STATUS "${PKG} was user-defined.")
    set(ANKICORETECH_${PKG} ${${PKG}})
  else()
    set(ANKICORETECH_${PKG} ${DEFAULT_${PKG}})
  endif(DEFINED ${PKG})
  # message(STATUS "Setting ${PKG} = ${ANKICORETECH_${PKG}}")
endforeach()

# Set the correct C++ language standard (including for Xcode):
if(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_VARIADIC_MAX=10 /D_CRT_SECURE_NO_WARNINGS /D_DLL")
elseif(CMAKE_GENERATOR MATCHES "Xcode")
  set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
elseif(LINUX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
endif(WIN32)

# Add networking libraries
if(WIN32)
set(NETWORKING_LIBS Ws2_32)
else()
set(NETWORKING_LIBS )
endif()

# So long as we're using full version names in our external libraries,
# lets not have to hardcode them all over the place:
set(OPENCV_DIR opencv-2.4.8)
set(GTEST_DIR gtest-1.7.0)

# I would love to figure out how to get find_package(OpenCV) to work,
# but so far, no luck.
#set(OpenCV_DIR "../coretech-external/build/Xcode/opencv-2.4.6.1")
#find_package(OpenCV REQUIRED)

# Specify which OpenCV libraries we're using here.  All of them.
# Everything will just get linked against them all the time, even
# if it doesn't need them. Is this overkill? Yes. But does it
# majorly simplify dealing with Cmake elsewhere? Absolutely.
set(OPENCV_LIBS
  opencv_core
  opencv_imgproc
  opencv_highgui
  opencv_calib3d
  opencv_objdetect
)

fix_opencv_lib_names(OPENCV_LIBS)

# Set up Matlab directories and mex extension:
include(FindMatlab) # This Find script doesn't seem to work on Mac!

# Since FindMatlab doesn't seem to work on Mac, do a naive search for Matlab in /Applications
# if MATLAB_ROOT_DIR is not already a defined variable.
# There's probably a better way to do this, but it seems to work.
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  if(NOT DEFINED MATLAB_ROOT_DIR)
    find_program(MEX_COMPILER mex)
    string(FIND ${MEX_COMPILER} "/Applications/" APP_SUBSTRING_POS)
    if(APP_SUBSTRING_POS MATCHES "0")
      # Strip off the end so that we only get /Applications/MATLAB_Rxxxxx.app
      string(REGEX REPLACE "/bin/mex" "" MATLAB_ROOT_DIR ${MEX_COMPILER})
      message(STATUS "Found matlab root at " ${MATLAB_ROOT_DIR})
    endif()
  endif()
endif()

if(NOT MATLAB_FOUND)
  message(STATUS "FindMatlab failed. Hard coding Matlab paths and mex settings.")
  if(WIN32)
    if(NOT DEFINED MATLAB_ROOT_DIR)
      # Try default matlab root paths
      if(IS_DIRECTORY "C:/Program Files/MATLAB/R2013a")
        set(MATLAB_ROOT "C:/Program Files/MATLAB/R2013a")
      elseif(IS_DIRECTORY "C:/Program Files (x86)/MATLAB/R2013a")
        set(MATLAB_ROOT "C:/Program Files (x86)/MATLAB/R2013a")
      elseif(IS_DIRECTORY "C:/Program Files/MATLAB/R2014a")
        set(MATLAB_ROOT "C:/Program Files/MATLAB/R2014a")
      elseif(IS_DIRECTORY "C:/Program Files (x86)/MATLAB/R2014a")
        set(MATLAB_ROOT "C:/Program Files (x86)/MATLAB/R2014a")
      else()
        set(MATLAB_ROOT "")
      endif()
    else()
      set(MATLAB_ROOT ${MATLAB_ROOT_DIR})
      message(STATUS "Using given MATLAB_ROOT ${MATLAB_ROOT}.")
    endif(NOT DEFINED MATLAB_ROOT_DIR)

    set(MATLAB_INCLUDE_DIR ${MATLAB_ROOT}/extern/include)
    set(MATLAB_LIBRARIES mx eng mex)
    set(MATLAB_MEX_LIBRARY_PATH ${MATLAB_ROOT}/extern/lib/win32/microsoft)
    set(MATLAB_MX_LIBRARY_PATH  ${MATLAB_ROOT}/extern/lib/win32/microsoft)
    set(MATLAB_ENG_LIBRARY_PATH ${MATLAB_ROOT}/extern/lib/win32/microsoft)
    set(MATLAB_ENG_LIBRARY libeng)
    set(MATLAB_MX_LIBRARY libmx)
    set(MATLAB_MEX_LIBRARY libmex)
#    set(ANKI_LIBRARIES CoreTech_Common_Embedded)
    set(ZLIB_LIBRARY zlib)
    set(CMD_COMMAND cmd /c)
    add_definitions(-DANKICORETECH_EMBEDDED_USE_OPENCV_SIMPLE_CONVERSIONS=1)
  elseif(LINUX)
    set(MATLAB_ROOT "")
    set(MATLAB_INCLUDE_DIR "")
    set(MATLAB_LIBRARIES)
    set(ZLIB_LIBRARY z pthread m)
    set(CMD_COMMAND)
    add_definitions(-DANKICORETECH_EMBEDDED_USE_OPENCV_SIMPLE_CONVERSIONS=0)
  else()
    if(NOT DEFINED MATLAB_ROOT_DIR)
      # Use default matlab root path
      if(IS_DIRECTORY /Applications/MATLAB_R2014a.app)
        set(MATLAB_ROOT /Applications/MATLAB_R2014a.app)
      elseif(IS_DIRECTORY /Applications/MATLAB_R2013a.app)
        set(MATLAB_ROOT /Applications/MATLAB_R2013a.app)
      else()
        set(MATLAB_ROOT "")
      endif()
    else()
      set(MATLAB_ROOT ${MATLAB_ROOT_DIR})
      message(STATUS "Using given MATLAB_ROOT ${MATLAB_ROOT}.")
    endif(NOT DEFINED MATLAB_ROOT_DIR)

    set(MATLAB_INCLUDE_DIR ${MATLAB_ROOT}/extern/include)
    set(MATLAB_LIBRARIES mx eng mex)
    set(MATLAB_MEX_LIBRARY_PATH ${MATLAB_ROOT}/bin/maci64)
    set(MATLAB_MX_LIBRARY_PATH  ${MATLAB_ROOT}/bin/maci64)
    set(MATLAB_ENG_LIBRARY_PATH ${MATLAB_ROOT}/bin/maci64)
    set(MATLAB_ENG_LIBRARY eng)
    set(MATLAB_MX_LIBRARY mx)
    set(MATLAB_MEX_LIBRARY mex)
#    set(ANKI_LIBRARIES CoreTech_Common CoreTech_Common_Embedded)
    set(CMD_COMMAND)
    set(ZLIB_LIBRARY z)
    set(CMD_COMMAND)
    add_definitions(-DANKICORETECH_EMBEDDED_USE_OPENCV_SIMPLE_CONVERSIONS=1)
  endif(WIN32)

  if(IS_DIRECTORY ${MATLAB_ROOT})
    set(MATLAB_FOUND 1)
  endif(IS_DIRECTORY ${MATLAB_ROOT})
endif(NOT MATLAB_FOUND)

if(MATLAB_FOUND)
  # set(MEX_COMPILER ${MATLAB_ROOT_DIR}/bin/mex)

  set(CMAKE_XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "${MATLAB_ENG_LIBRARY_PATH}")

  # Set the mex extension using Matlab's "mexext" script:
  # (Does this exist on Windows machines?)
  set(MATLAB_BIN_DIR "${MATLAB_ROOT}/bin/")
  execute_process(COMMAND ${CMD_COMMAND} "${MATLAB_BIN_DIR}/mexext" OUTPUT_VARIABLE MATLAB_MEXEXT)
  string(STRIP "${MATLAB_MEXEXT}" MATLAB_MEXEXT)

  message(STATUS "Using Matlab in ${MATLAB_ROOT} with mex extension ${MATLAB_MEXEXT}.")
else ()
     message(STATUS "Disabling USE_MATLAB")
     set(ANKICORETECH_USE_MATLAB 0)
     set(ANKICORETECH_EMBEDDED_USE_MATLAB 0)
endif(MATLAB_FOUND)

# After this point nothing else should update the ANKICORETECH_USE_* settings.
# We will now use them to add -D compile switches.
foreach(PKG ${PKG_OPTIONS})
        add_definitions(-DANKICORETECH_${PKG}=${ANKICORETECH_${PKG}})
        message(STATUS "adding definitions -DANKICORETECH_${PKG}=${ANKICORETECH_${PKG}}")
endforeach()

# Add all required external embedded libraries to the list for the linker
set(ALL_EMBEDDED_LIBRARIES ${ZLIB_LIBRARY})

if(${ANKICORETECH_EMBEDDED_USE_MATLAB})
  set(ALL_EMBEDDED_LIBRARIES ${ALL_EMBEDDED_LIBRARIES} ${MATLAB_ENG_LIBRARY} ${MATLAB_MX_LIBRARY})
endif()

if(${ANKICORETECH_EMBEDDED_USE_OPENCV})
  set(ALL_EMBEDDED_LIBRARIES ${ALL_EMBEDDED_LIBRARIES} ${OPENCV_LIBS})
endif()

if(${ANKICORETECH_EMBEDDED_USE_GTEST})
  set(ALL_EMBEDDED_LIBRARIES ${ALL_EMBEDDED_LIBRARIES} gtest)
endif()

set(ALL_EMBEDDED_LIBRARIES ${ALL_EMBEDDED_LIBRARIES} ${NETWORKING_LIBS})

project(${PROJECT_NAME})

# Make sure folder-based organization of targets is enabled
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# This is a hack, to get around some new cmake requirements
if(WIN32)
  STRING(REPLACE "INCREMENTAL" "INCREMENTAL:NO" replacementFlags ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
  SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "/INCREMENTAL:NO ${replacementFlags}" )

  STRING(REPLACE "INCREMENTAL" "INCREMENTAL:NO" replacementFlags3 ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO})
  SET(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/INCREMENTAL:NO ${replacementFlags3}" )
endif()

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
if(NOT DEFINED EXTERNAL_DIR)
  set(EXTERNAL_DIR ${PROJECT_SOURCE_DIR}/../coretech-external)
endif(NOT DEFINED EXTERNAL_DIR)

set(OPENCV_MODULES_DIR ${EXTERNAL_DIR}/${OPENCV_DIR}/modules)
include_directories(
  ${EXTERNAL_DIR}/${OPENCV_DIR}/include
  ${EXTERNAL_DIR}/${OPENCV_DIR}/3rdparty/zlib
  ${EXTERNAL_DIR}/build/${OPENCV_DIR}/3rdparty/zlib
  ${EXTERNAL_DIR}/${GTEST_DIR}/include
  ${EXTERNAL_DIR}/jsoncpp
  ${MATLAB_INCLUDE_DIR}
)

include_directories(${PROJECT_SOURCE_DIR}/robot/test)

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
  message(STATUS "Appending ${CMAKE_BUILD_TYPE} to output directories since Unix Makefiles are being used.")
  set(BUILD_TYPE_DIR ${CMAKE_BUILD_TYPE})
else()
  set(BUILD_TYPE_DIR ./)
endif()

# Store our libraries in, e.g., coretech-vision/build/Xcode/lib/Debug
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/${CMAKE_GENERATOR}/lib/${BUILD_TYPE_DIR})

# Store our executables such as tests in, e.g., coretech-vision/build/Xcode/bin/Debug
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/${CMAKE_GENERATOR}/bin/${BUILD_TYPE_DIR})

link_directories(
  ${LIBRARY_OUTPUT_PATH}
  ${EXTERNAL_DIR}/build/${CMAKE_GENERATOR}/lib/${BUILD_TYPE_DIR}
)

if(ANKICORETECH_USE_MATLAB)
  link_directories(
    ${MATLAB_MEX_LIBRARY_PATH}
    ${MATLAB_MX_LIBRARY_PATH}
    ${MATLAB_ENG_LIBRARY_PATH}
  )
endif()

# Of course, OpenCV is special... (it won't (?) obey the OUTPUT_PATHs specified
# above, so we'll add link directories for where it put its products)
if(WIN32)
  link_directories(${EXTERNAL_DIR}/build/${OPENCV_DIR}/lib/Debug)
  link_directories(${EXTERNAL_DIR}/build/${OPENCV_DIR}/lib/RelWithDebInfo)
  link_directories(${EXTERNAL_DIR}/build/${OPENCV_DIR}/3rdparty/lib/Debug)
  link_directories(${EXTERNAL_DIR}/build/${OPENCV_DIR}/3rdparty/lib/RelWithDebInfo)
else()
  link_directories(${EXTERNAL_DIR}/build/${OPENCV_DIR}/lib)
  link_directories(${EXTERNAL_DIR}/build/${OPENCV_DIR}/3rdparty/lib)
endif(WIN32)


endmacro(ankiProject)

#
# A helper macro (read: hack) for appending "248d" to the opencv library names.
#
function(fix_opencv_lib_names NAMES)

if(WIN32)
  foreach(NAME IN LISTS ${NAMES})
    list(APPEND ${NAMES}_OPTIMIZED_TMP optimized ${NAME}248)
    list(APPEND ${NAMES}_DEBUG_TMP debug ${NAME}248d)
  endforeach()

  set(${NAMES} "${${NAMES}_OPTIMIZED_TMP}" "${${NAMES}_DEBUG_TMP}" PARENT_SCOPE)
endif(WIN32)

endfunction(fix_opencv_lib_names)

#
# A helper macro for building mex files and linking them against our
# "standard" set of libraries, namely:
#   opencv_core, CoreTech_Common, and CoreTech_Common_Embedded.
#
# You can link against others by setting the variable "MEX_LINK_LIBRARIES"
# before calling this macro, e.g.:
#  set(MEX_LINK_LIBRARIES opencv_highgui CoreTech_Vision)
#
# I'm not 100% sure this totally perfect, but so far it seems to be working.
# Tweaking and improvements are welcome.  See also:
#   http://www.cmake.org/Wiki/CMake:MatlabMex
#
macro(build_mex MEX_FILE)

if( MATLAB_FOUND AND (ANKICORETECH_USE_MATLAB OR ANKICORETECHEMBEDDED_USE_MATLAB) )
  #message(STATUS "Adding mex file ${MEX_FILE}, linked against ${MEX_LINK_LIBRARIES}")

  set(CC ${MEX_COMPILER})
  set(CXX ${MEX_COMPILER})
  unset(CMAKE_CXX_FLAGS)
  unset(CMAKE_C_FLAGS)
  set(CMAKE_CXX_FLAGS "-largeArrayDims")

  # If not told otherwise, store our mex binaries in, e.g., coretech-vision/build/mex
  if(NOT DEFINED MEX_OUTPUT_PATH)
    set(MEX_OUTPUT_PATH ${PROJECT_BINARY_DIR}/mex)
  endif(NOT DEFINED MEX_OUTPUT_PATH)

  get_filename_component(OUTPUT_NAME ${MEX_FILE} NAME_WE)

  if(NOT DEFINED NO_MEX_WRAPPERS)
    set(MEX_WRAPPER_FILE ${CORETECH_ROOT_DIR}/common/matlab/mex/mexWrappers.cpp)
  endif(NOT DEFINED NO_MEX_WRAPPERS)

  add_library(${OUTPUT_NAME} SHARED
    ${MEX_FILE}
    ${MEX_WRAPPER_FILE}
    ${CORETECH_ROOT_DIR}/common/matlab/mex/mexFunction.def
  )

  # Put mex binaries in MEX_OUTPUT_PATH
  foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )

    # First for the generic no-config case (e.g. with mingw)
    set_target_properties( ${OUTPUT_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${MEX_OUTPUT_PATH} )
    set_target_properties( ${OUTPUT_NAME} PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY ${MEX_OUTPUT_PATH} )
    set_target_properties( ${OUTPUT_NAME} PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY ${MEX_OUTPUT_PATH} )
#
#    # Second, for multi-config builds (e.g. msvc / Xcode)
#    set_target_properties( ${OUTPUT_NAME} PROPERTIES
#      RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${MEX_OUTPUT_PATH} )
#    set_target_properties( ${OUTPUT_NAME} PROPERTIES
#      LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${MEX_OUTPUT_PATH} )
#    set_target_properties( ${OUTPUT_NAME} PROPERTIES
#      ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${MEX_OUTPUT_PATH} )
#
  endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

  # Prevent mex outputs from ending up in Debug/Release subdirectories, by
  # moving them up one directory.
  # (NOTE: it would be better to use the approach commented out in the loop
  #  above, which just sets the final build directory directly for each
  #  configuration type, but that is creating crazy/distracting
  #  Xcode warnings -- even though it does actually work.)
  add_custom_command(TARGET ${OUTPUT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${OUTPUT_NAME}> $<TARGET_FILE_DIR:${OUTPUT_NAME}>/..
    COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:${OUTPUT_NAME}>
  )

  if(DEFINED MEX_LINK_LIBRARIES)
    #foreach(LIB ${MEX_LINK_LIBRARIES})
        #target_link_libraries(${OUTPUT_NAME} ${LIB})
    #endforeach()
    #message(${MEX_LINK_LIBRARIES})
    target_link_libraries(${OUTPUT_NAME} ${MEX_LINK_LIBRARIES})
  endif()

  target_link_libraries(${OUTPUT_NAME}
    ${ZLIB_LIBRARY}
    jsoncpp
  )

  if(ANKICORETECH_USE_MATLAB)
    target_link_libraries(${OUTPUT_NAME}
      ${MATLAB_MEX_LIBRARY}
      ${MATLAB_MX_LIBRARY}
      ${MATLAB_ENG_LIBRARY}
    )
  endif()

  #message(STATUS "For MEX file ${OUTPUT_NAME}, linking against ${MEX_LINK_LIBRARIES}")

  set_target_properties(${OUTPUT_NAME} PROPERTIES
    # Provide a #define so we can know when we're building a mex file
    COMPILE_DEFINITIONS "ANKI_MEX_BUILD;MATLAB_MEX_FILE"
    PREFIX ""
    SUFFIX ".${MATLAB_MEXEXT}"
    FOLDER "Matlab Mex"
  )

endif()

endmacro(build_mex)
