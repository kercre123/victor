# cozmoEngine/CMakeLists.txt

cmake_minimum_required(VERSION 3.0.0)

project(CozmoEngineMex)

if (NOT DEFINED CORETECH_BINARY_DIR)
  message(FATAL_ERROR "ERROR: CORETECH_BINARY_DIR must be defined!")
endif(NOT DEFINED CORETECH_BINARY_DIR)

set(CORETECH_ROOT_DIR "${PROJECT_SOURCE_DIR}/coretech")

# Check that path to coretech-external is defined in environment var
if (NOT DEFINED ENV{CORETECH_EXTERNAL_DIR})
  message(FATAL_ERROR "ERROR: CORETECH_EXTERNAL_DIR is undefined")
endif(NOT DEFINED ENV{CORETECH_EXTERNAL_DIR})
set(CORETECH_EXTERNAL_DIR $ENV{CORETECH_EXTERNAL_DIR})

set(EXTERNAL_BUILD_DIR ${CORETECH_EXTERNAL_DIR}/build)

message(STATUS "External build dir = ${EXTERNAL_BUILD_DIR}")

# Set the correct C++ language standard (including for Xcode):
if(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_VARIADIC_MAX=10 /D_CRT_SECURE_NO_WARNINGS /D_DLL")
elseif(CMAKE_GENERATOR MATCHES "Xcode")
  set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
elseif(LINUX)
  if(CPU_A7)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -save-temps -g -mcpu=cortex-a7 -mfpu=neon-vfpv4 -ftree-vectorize -funroll-loops")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -save-temps -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -save-temps -g -mcpu=cortex-a7 -mfpu=neon-vfpv4")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -save-temps -g -mcpu=cortex-a7 -mfpu=neon-vfpv4 -ffast-math")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -save-temps -g -mcpu=cortex-a7 -mfpu=neon-vfpv4 -Ofast")
    if(VERBOSE_AUTOVECTORIZE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopt-info-vec-optimized")
    endif()
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -save-temps -g")
  endif()
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
endif(WIN32)

# So long as we're using full version names in our external libraries,
# lets not have to hardcode them all over the place:
set(OPENCV_DIR opencv-2.4.8)

# I would love to figure out how to get find_package(OpenCV) to work,
# but so far, no luck.
#set(OpenCV_DIR "../coretech-external/build/Xcode/opencv-2.4.6.1")
#find_package(OpenCV REQUIRED)


message(STATUS "Setting OpenCV libs required")


# Specify which OpenCV libraries we're using here.  All of them.
# Everything will just get linked against them all the time, even
# if it doesn't need them. Is this overkill? Yes. But does it
# majorly simplify dealing with Cmake elsewhere? Absolutely.
set(OPENCV_LIBS
  opencv_core
  opencv_imgproc
  opencv_highgui
  opencv_calib3d
  opencv_contrib
  opencv_objdetect
  opencv_video
  opencv_features2d
)


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

set(MATLAB_FOUND 0)

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
      elseif(IS_DIRECTORY "C:/Program Files (x86)/MATLAB/R2014b")
        set(MATLAB_ROOT "C:/Program Files (x86)/MATLAB/R2014b")
      else()
        message(STATUS "No known Matlab path exists, setting empty MATLAB_ROOT.")
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
  elseif(LINUX)
    set(MATLAB_ROOT "")
    set(MATLAB_INCLUDE_DIR "")
    set(MATLAB_LIBRARIES)
    if(EMBEDDED_USE_OPENCV)
      set(ZLIB_LIBRARY z pthread m rt)
    else()
      set(ZLIB_LIBRARY m rt)
    endif()
    
    set(CMD_COMMAND)
  else()
    if(NOT DEFINED MATLAB_ROOT_DIR)
      # Use default matlab root path
      if(IS_DIRECTORY /Applications/MATLAB_R2015a.app)
        set(MATLAB_ROOT /Applications/MATLAB_R2015a.app)
      elseif(IS_DIRECTORY /Applications/MATLAB_R2014b.app)
        set(MATLAB_ROOT /Applications/MATLAB_R2014b.app)
      elseif(IS_DIRECTORY /Applications/MATLAB_R2014a.app)
        set(MATLAB_ROOT /Applications/MATLAB_R2014a.app)
      elseif(IS_DIRECTORY /Applications/MATLAB_R2013a.app)
        set(MATLAB_ROOT /Applications/MATLAB_R2013a.app)
      else()
        message(STATUS "No known Matlab path exists, setting empty MATLAB_ROOT.")
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

# We will now use them to add -D compile switches.
add_definitions(-DANKICORETECH_USE_MATLAB=1)
add_definitions(-DANKICORETECH_USE_OPENCV=1)
add_definitions(-DANKICORETECH_EMBEDDED_USE_MATLAB)
add_definitions(-DANKICORETECH_EMBEDDED_USE_OPENCV)
add_definitions(-DANKI_MEX_BUILD)

# Add all required external embedded libraries to the list for the linker
set(ALL_EMBEDDED_LIBRARIES 
  ${ZLIB_LIBRARY}
  ${ALL_EMBEDDED_LIBRARIES} 
  ${MATLAB_ENG_LIBRARY} 
  ${MATLAB_MX_LIBRARY}
  ${OPENCV_LIBS}
)


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


set(OPENCV_MODULES_DIR ${CORETECH_EXTERNAL_DIR}/${OPENCV_DIR}/modules)


# Add the include directory for each OpenCV module:
file(GLOB OPENCV_MODULES RELATIVE ${OPENCV_MODULES_DIR} ${OPENCV_MODULES_DIR}/*)
foreach(OPENCV_MODULE ${OPENCV_MODULES})
  if(IS_DIRECTORY ${OPENCV_MODULES_DIR}/${OPENCV_MODULE})
    include_directories(${OPENCV_MODULES_DIR}/${OPENCV_MODULE}/include)
  endif()
endforeach()

set(ANKI_UTIL_DIR "${PROJECT_SOURCE_DIR}/tools/anki-util")

set(COZMO_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

set(COZMO_BASESTATION_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/basestation/include)

file(GLOB COZMO_INCLUDE_FILES "${COZMO_INCLUDE_DIR}/anki/cozmo/shared/*.h")

# This sets the root path for the PlatformPathManager when not on iOS
if (NOT SYSTEM_ROOT_PATH_DEFINED)
  add_definitions(-DSYSTEM_ROOT_PATH=${PROJECT_SOURCE_DIR})
  set(SYSTEM_ROOT_PATH_DEFINED 1)
endif(NOT SYSTEM_ROOT_PATH_DEFINED)

message(STATUS "Setting include directories")

include_directories(
  ${COZMO_INCLUDE_DIR}
  ${CORETECH_ROOT_DIR}/common/include
  ${CORETECH_ROOT_DIR}/messaging/include
  ${CORETECH_ROOT_DIR}/vision/include
  ${CORETECH_ROOT_DIR}/planning/include
  ${CORETECH_ROOT_DIR}/common/tools/threads
  ${CLAD_DIR}/support/cpp/include
  "${ANKI_UTIL_DIR}/source"
  "${ANKI_UTIL_DIR}/source/3rd"
  "${ANKI_UTIL_DIR}/source/3rd/jsoncpp"
  "${ANKI_UTIL_DIR}/source/anki"
  "${ANKI_UTIL_DIR}/libs/packaged/include"
  ${CORETECH_EXTERNAL_DIR}/${OPENCV_DIR}/include
  ${MATLAB_INCLUDE_DIR}
)

if(WIN32)
  include_directories(${CORETECH_EXTERNAL_DIR}/${OPENCV_DIR}/3rdparty/zlib)
  include_directories(${EXTERNAL_BUILD_DIR}/${OPENCV_DIR}/3rdparty/zlib)
endif()


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
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/${CMAKE_GENERATOR}/lib/${BUILD_TYPE_DIR})

# Store our executables such as tests in, e.g., coretech-vision/build/Xcode/bin/Debug
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/${CMAKE_GENERATOR}/bin/${BUILD_TYPE_DIR})

link_directories(
  ${LIBRARY_OUTPUT_PATH}
  ${EXTERNAL_BUILD_DIR}/${CMAKE_GENERATOR}/lib/${BUILD_TYPE_DIR}
  ${MATLAB_MEX_LIBRARY_PATH}
  ${MATLAB_MX_LIBRARY_PATH}
  ${MATLAB_ENG_LIBRARY_PATH}
  ${CORETECH_BINARY_DIR}
)

# Of course, OpenCV is special... (it won't (?) obey the OUTPUT_PATHs specified
# above, so we'll add link directories for where it put its products)
if(WIN32)
  link_directories(${EXTERNAL_BUILD_DIR}/${OPENCV_DIR}/lib/Debug)
  link_directories(${EXTERNAL_BUILD_DIR}/${OPENCV_DIR}/lib/RelWithDebInfo)
  link_directories(${EXTERNAL_BUILD_DIR}/${OPENCV_DIR}/3rdparty/lib/Debug)
  link_directories(${EXTERNAL_BUILD_DIR}/${OPENCV_DIR}/3rdparty/lib/RelWithDebInfo)
else()
  link_directories(${EXTERNAL_BUILD_DIR}/${OPENCV_DIR}/lib)
  link_directories(${EXTERNAL_BUILD_DIR}/${OPENCV_DIR}/3rdparty/lib)
endif(WIN32)

set(CMAKE_CXX_FLAGS "-Wno-unused-function -Wno-overloaded-virtual -Wno-deprecated-declarations -Wno-unused-variable")




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

if( MATLAB_FOUND )
  #message(STATUS "Adding mex file ${MEX_FILE}, linked against ${MEX_LINK_LIBRARIES}")

  set(CC ${MEX_COMPILER})
  set(CXX ${MEX_COMPILER})
  unset(CMAKE_CXX_FLAGS)
  unset(CMAKE_C_FLAGS)
  # set(CMAKE_CXX_FLAGS "-largeArrayDims")

  # If not told otherwise, store our mex binaries in, e.g., coretech-vision/build/mex
  if(NOT DEFINED MEX_OUTPUT_PATH)
    set(MEX_OUTPUT_PATH ${PROJECT_BINARY_DIR}/mex)
  endif(NOT DEFINED MEX_OUTPUT_PATH)

  get_filename_component(OUTPUT_NAME ${MEX_FILE} NAME_WE)

  if(NOT DEFINED NO_MEX_WRAPPERS)
    set(MEX_WRAPPER_FILES 
        "${CORETECH_ROOT_DIR}/common/matlab/mex/mexWrappers.cpp"
        "${CORETECH_ROOT_DIR}/common/shared/src/matlabConverters.cpp"
        "${CORETECH_ROOT_DIR}/common/shared/src/sharedMatlabInterface.cpp"
      "${CORETECH_ROOT_DIR}/common/robot/src/matlabInterface.cpp")
  endif(NOT DEFINED NO_MEX_WRAPPERS)

  add_library(${OUTPUT_NAME} SHARED
    ${MEX_FILE}
    ${MEX_WRAPPER_FILES}
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

  target_link_libraries(${OUTPUT_NAME}
    ${MATLAB_MEX_LIBRARY}
    ${MATLAB_MX_LIBRARY}
    ${MATLAB_ENG_LIBRARY}
    util
    utilEmbedded
  )

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


message(STATUS "Adding subdirectories")

# Finally, actually add the directories containing the mex source code!

add_subdirectory(matlab/mex)
add_subdirectory(coretech/vision/matlab/mex)
add_subdirectory(coretech/vision/matlab/CameraCapture)
add_subdirectory(coretech/vision/robot/mex)
add_subdirectory(coretech/common/matlab/mex)
add_subdirectory(coretech/common/robot/mex)



