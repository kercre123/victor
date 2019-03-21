#****************************************************************************
# Copyright (C) 2018 pmdtechnologies ag & Infineon Technologies
#
# THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
# KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
# PARTICULAR PURPOSE.
#
#****************************************************************************

# ===================================================================================
#  The Royale CMake configuration file
#
#  Usage from an external project:
#    In your CMakeLists.txt, add these lines:
#
#    FIND_PACKAGE(royale REQUIRED)
#    LINK_DIRECTORIES(${royale_LIB_DIR)
#    TARGET_LINK_LIBRARIES(MY_TARGET_NAME "${royale_LIBS}")
#
#   For Windows also add: (where "MY_TARGET_NAME" is depending on your project)
#
#    add_custom_command(
#       TARGET MY_TARGET_NAME
#       POST_BUILD
#       COMMAND ${CMAKE_COMMAND} -E copy "${royale_INSTALL_PATH}/bin/royale.dll"  $<TARGET_FILE_DIR:MY_TARGET_NAME>)
#
#
#   For OS X also add: (where "MY_TARGET_NAME" is depending on your project)
#
#   add_custom_command(
#       TARGET MY_TARGET_NAME
#       POST_BUILD
#       COMMAND ${CMAKE_COMMAND} -E copy "${royale_INSTALL_PATH}/bin/libroyale.dylib"  $<TARGET_FILE_DIR:MY_TARGET_NAME>)
#
#   add_custom_command(
#       TARGET MY_TARGET_NAME
#       POST_BUILD
#       COMMAND ${CMAKE_COMMAND} -E copy "${royale_INSTALL_PATH}/bin/libroyale.${royale_VERSION}.dylib"  $<TARGET_FILE_DIR:MY_TARGET_NAME>)
#

FUNCTION(ROYALE_REMOVE_FROM_STRING stringname value)
    IF(DEFINED "${stringname}")
        IF(NOT "${${stringname}}" STREQUAL "")
            STRING( REGEX REPLACE "${value}" "" "${stringname}" "${${stringname}}" )
            SET("${stringname}" "${${stringname}}" PARENT_SCOPE)
        ELSE()
            MESSAGE("string with name ${stringname} is empty!")
        ENDIF()
    ELSE()
        MESSAGE("no string with name ${stringname}")
    ENDIF()
ENDFUNCTION()

# Extract the directory where *this* file has been installed (determined at cmake run-time)
get_filename_component(royale_CONFIG_PATH "${CMAKE_CURRENT_LIST_FILE}" PATH CACHE)

set(royale_INSTALL_PATH "${royale_CONFIG_PATH}/..")

# Get the absolute path with no ../.. relative marks, to eliminate implicit linker warnings
if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_LESS 2.8)
  get_filename_component(royale_INSTALL_PATH "${royale_INSTALL_PATH}" ABSOLUTE)
else()
  get_filename_component(royale_INSTALL_PATH "${royale_INSTALL_PATH}" REALPATH)
endif()

# Provide the include directories to the caller
set(royale_INCLUDE_DIRS "${royale_INSTALL_PATH}/include;${royale_INSTALL_PATH}/include/royaleCAPI")
include_directories(${royale_INCLUDE_DIRS})

set(royale_LIBS "royale")

if (UNIX)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# Provide the libs directories to the caller
set(royale_LIB_DIR  CACHE PATH "Path where Royale libraries are located")
mark_as_advanced(FORCE royale_LIB_DIR royale_CONFIG_PATH)

if (WIN32)
	set(royale_LIB_DIR "${royale_INSTALL_PATH}/lib")
else ()
	set(royale_LIB_DIR "${royale_INSTALL_PATH}/bin")
endif ()
set(royale_LIBRARIES ${royale_LIBS})

if (WIN32)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_WINDOWS /W3 /GR /EHsc /fp:fast /arch:SSE2 /Oy /Ot /D_VARIADIC_MAX=10 /DNOMINMAX /MD" CACHE STRING "" FORCE)
    SET(CMAKE_CXX_FLAGS_DEBUG "/Zi /Ob0 /Od /MD /RTC1" CACHE STRING "" FORCE)
    SET(CMAKE_CXX_FLAGS_MINSIZEREL "/O1 /Ob1 /D NDEBUG /MD" CACHE STRING "" FORCE)
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Zi /O2 /Ob1 /D NDEBUG /MD" CACHE STRING "" FORCE)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # Remove SSE2 flag for 64Bit as it causes a warning
        ROYALE_REMOVE_FROM_STRING(CMAKE_CXX_FLAGS " /arch:SSE2")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "" FORCE)
    endif()
elseif(APPLE)
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wall -Wsign-compare -Wuninitialized -Wunused -Wno-deprecated-declarations -std=c++11" CACHE STRING "" FORCE)
   SET(CMAKE_MACOSX_RPATH TRUE CACHE BOOL "")
   SET(CMAKE_BUILD_WITH_INSTALL_RPATH OFF CACHE BOOL "")
   SET(CMAKE_INSTALL_RPATH "@loader_path" CACHE STRING "")
elseif(UNIX)
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++0x -Wall -Wconversion" CACHE STRING "" FORCE)
   SET(CMAKE_BUILD_WITH_INSTALL_RPATH OFF CACHE BOOL "")
   SET(CMAKE_INSTALL_RPATH "\$ORIGIN/" CACHE STRING "")
endif(WIN32)

SET(royale_VERSION 3.21.1)
SET(royale_VERSION_MAJOR  3)
SET(royale_VERSION_MINOR  21)
SET(royale_VERSION_PATCH  1)
SET(royale_VERSION_BUILD  70)

macro(COPY_ROYALE_LIBS copytarget)
if (WIN32)
    add_custom_command(
        TARGET ${copytarget}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${royale_INSTALL_PATH}/bin/royale.dll"  $<TARGET_FILE_DIR:${copytarget}>)

    add_custom_command(
        TARGET ${copytarget}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${royale_INSTALL_PATH}/bin/spectre3.dll"  $<TARGET_FILE_DIR:${copytarget}>)
endif (WIN32)

if (APPLE)
    add_custom_command(
        TARGET ${copytarget}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${royale_INSTALL_PATH}/bin/libroyale.dylib" $<TARGET_FILE_DIR:${copytarget}>)

    add_custom_command(
        TARGET ${copytarget}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${royale_INSTALL_PATH}/bin/libroyale.${royale_VERSION}.dylib" $<TARGET_FILE_DIR:${copytarget}>)

    add_custom_command(
        TARGET ${copytarget}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${royale_INSTALL_PATH}/bin/libspectre3.dylib" $<TARGET_FILE_DIR:${copytarget}>)
endif (APPLE)
endmacro(COPY_ROYALE_LIBS)

