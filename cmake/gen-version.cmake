#
# gen-version.cmake
#
# Copyright Anki, Inc. 2018
#
# Generates a files containing info in  <build>/etc/{version,revision}

#
# <build>/etc/version
#

#
# The version number format is:
#
# X.Y.Z<build_type>[_<build_tag>]
# 
# X = major version
# Y = minor version
# Z = incremental build number
# build_type = dev (d) or empty (release)
# build_tag = optional tag specifier for build

# read base version from VERSION file
file(READ ${CMAKE_SOURCE_DIR}/VERSION BASE_VERSION)
string(STRIP ${BASE_VERSION} BASE_VERSION)

# ANKI_BUILD_VERSION  contains the build counter
set(ANKI_BUILD_VERSION 0)
if (DEFINED ENV{ANKI_BUILD_VERSION})
    set(ANKI_BUILD_VERSION $ENV{ANKI_BUILD_VERSION})
endif()

# default to "dev" build flavor
set(ANKI_BUILD_TYPE "d")
if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set(ANKI_BUILD_TYPE "")
endif()

# if ANKI_BUILD_TAG is defined, append it to the version number with `_` prefex
set(ANKI_BUILD_TAG "")
if (DEFINED ENV{ANKI_BUILD_TAG})
    set(ANKI_BUILD_VERSION "_$ENV{ANKI_BUILD_TAG}")
endif()

configure_file(${CMAKE_SOURCE_DIR}/templates/cmake/version.in
               ${CMAKE_BINARY_DIR}/etc/version
               @ONLY)


#
# <build>/etc/revision
#

# if ANKI_BUILD_VICTOR_REV contains the git revision, use it
set(ANKI_BUILD_VICTOR_REV "")
if (DEFINED ENV{ANKI_BUILD_VICTOR_REV})
    set(ANKI_BUILD_VICTOR_REV $ENV{ANKI_BUILD_VICTOR_REV})
else()
    execute_process(
      COMMAND git rev-parse --short HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE ANKI_BUILD_VICTOR_REV
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

configure_file(${CMAKE_SOURCE_DIR}/templates/cmake/revision.in
               ${CMAKE_BINARY_DIR}/etc/revision
               @ONLY)
