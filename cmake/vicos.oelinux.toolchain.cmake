cmake_minimum_required(VERSION 3.6.0)

# CMake invokes the toolchain file twice during the first build, but only once
# during subsequent rebuilds. This was causing the various flags to be added
# twice on the first build, and on a rebuild ninja would see only one set of the
# flags and rebuild the world.
# https://github.com/android-ndk/ndk/issues/323
if(VICOS_TOOLCHAIN_INCLUDED)
    return()
endif(VICOS_TOOLCHAIN_INCLUDED)

set(VICOS_TOOLCHAIN_INCLUDED true)

# Touch toolchain variable to suppress "unused variable" warning.
# This happens if CMake is invoked with the same command line the second time.
if(CMAKE_TOOLCHAIN_FILE)
endif()

# We only support clang
set(VICOS_TOOLCHAIN clang)

if(NOT VICOS_SDK)
	if(DEFINED ENV{VICOS_SDK} AND IS_DIRECTORY "$ENV{VICOS_SDK}")
		set(VICOS_SDK "$ENV{VICOS_SDK}")
    elseif(DEFINED ENV{VICOS_SDK_HOME} AND IS_DIRECTORY "$ENV{VICOS_SDK_HOME}")
		set(VICOS_SDK "$ENV{VICOS_SDK_HOME}")
	elseif(DEFINED ENV{_VICOS_SDK} AND IS_DIRECTORY "$ENV{_VICOS_SDK}")
		set(VICOS_SDK "$ENV{_VICOS_SDK}")
    else()
        message(FATAL_ERROR "Invalid vicos SDK. Define VICOS_SDK_HOME the environment or set the VICOS_SDK variable.")
    endif()
endif()

set(ENV{_VICOS_SDK} "${VICOS_SDK}")

set(VICOS_CPP_FEATURES rtti exceptions)

file(TO_CMAKE_PATH "${VICOS_SDK}" VICOS_SDK)

# Standard cross-compiling stuff.
set(VICOS TRUE)

set(CMAKE_SYSTEM_NAME Linux)

# Sysroot.
set(CMAKE_SYSROOT "${VICOS_SDK}/sysroot")

# set system version to OE linux kernel version
set(CMAKE_SYSTEM_VERSION "3.18")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ABI.
#set(CMAKE_ANDROID_ARCH_ABI ${VICOS_ABI})

# This is used as "prefix" for toolchain binaries
set(VICOS_TOOLCHAIN_NAME arm-oe-linux-gnueabi)

# This is used to construct the path prefix for the toolchain location
set(VICOS_TOOLCHAIN_ROOT arm-oe-linux-gnueabi)

# uname -p on vicos returns unknown
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

set(VICOS_LLVM_TRIPLE arm-oe-linux-gnueabi)

# Toolchain.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    set(VICOS_HOST_TAG linux-x86_64)
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL Darwin)
    set(VICOS_HOST_TAG darwin-x86_64)
endif()

set(VICOS_TOOLCHAIN_ROOT "${VICOS_SDK}/prebuilt")
set(VICOS_TOOLCHAIN_PREFIX "${VICOS_TOOLCHAIN_ROOT}/bin/${VICOS_TOOLCHAIN_NAME}-")

# BRC these defines are important
set(VICOS_LLVM_TOOLCHAIN_PREFIX "${VICOS_SDK}/prebuilt/bin/${VICOS_TOOLCHAIN_NAME}-")
set(VICOS_C_COMPILER   "${VICOS_LLVM_TOOLCHAIN_PREFIX}clang${VICOS_TOOLCHAIN_SUFFIX}")
set(VICOS_CXX_COMPILER "${VICOS_LLVM_TOOLCHAIN_PREFIX}clang++${VICOS_TOOLCHAIN_SUFFIX}")
set(CMAKE_C_COMPILER_TARGET   ${VICOS_LLVM_TRIPLE})
set(CMAKE_CXX_COMPILER_TARGET ${VICOS_LLVM_TRIPLE})
set(CMAKE_C_COMPILER_EXTERNAL_TOOLCHAIN   "${VICOS_TOOLCHAIN_ROOT}")
set(CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN "${VICOS_TOOLCHAIN_ROOT}")


set(VICOS_COMPILER_FLAGS)
set(VICOS_COMPILER_FLAGS_CXX)
set(VICOS_COMPILER_FLAGS_DEBUG)
set(VICOS_COMPILER_FLAGS_RELEASE)
set(VICOS_LINKER_FLAGS)
set(VICOS_LINKER_FLAGS_EXE)

# Generic flags.
list(APPEND VICOS_COMPILER_FLAGS
	-gsplit-dwarf
    -DVICOS
	-ffunction-sections
	-fdata-sections
	-funwind-tables
	-fstack-protector-strong
#  -flto
#  -fvisibility=hidden
#  -fsanitize=cfi
	-no-canonical-prefixes)
list(APPEND VICOS_COMPILER_FLAGS_CXX
	-fno-exceptions
	-fno-rtti)
list(APPEND VICOS_COMPILER_FLAGS_RELEASE
  -D_FORTIFY_SOURCE=2)
list(APPEND VICOS_LINKER_FLAGS
	-Wl,--build-id
	-Wl,--gdb-index
	-Wl,--warn-shared-textrel
	-Wl,--gc-sections
	-Wl,--fatal-warnings)
list(APPEND VICOS_LINKER_FLAGS_EXE
	-Wl,-z,nocopyreloc)

# Debug and release flags.
list(APPEND VICOS_COMPILER_FLAGS_DEBUG
	-O0
        -fno-limit-debug-info)
list(APPEND VICOS_COMPILER_FLAGS_RELEASE
	-Os
        -DNDEBUG)

# Toolchain and ABI specific flags.
list(APPEND VICOS_COMPILER_FLAGS
	-march=armv7-a
	-mfloat-abi=softfp
	-mfpu=vfpv3-d16)
list(APPEND VICOS_LINKER_FLAGS
	-Wl,--fix-cortex-a8)

# Disable integrated-as for better compatibility.
# list(APPEND ANDROID_COMPILER_FLAGS
#	-fno-integrated-as)

# STL specific flags for libc++
set(VICOS_STL_PREFIX llvm-libc++)
list(APPEND VICOS_LINKER_FLAGS
	-Wl,--exclude-libs,libunwind.a
	-lc
	-latomic
	-lpthread)
list(APPEND VICOS_COMPILER_FLAGS_CXX
	-stdlib=libc++
	-std=c++14)

set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES "")

# Add libc++_shared.so
set(VICOS_CXX_STANDARD_LIBRARIES "${VICOS_SDK}/sysroot/usr/lib/libc++.so")

# Add libunwind
set(VICOS_CXX_STANDARD_LIBRARIES "${VICOS_SDK}/sysroot/usr/lib/libunwind.a")

set(CMAKE_C_STANDARD_LIBRARIES_INIT "-lm")
set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "${CMAKE_C_STANDARD_LIBRARIES_INIT}")
if(VICOS_CXX_STANDARD_LIBRARIES)
    string(REPLACE ";" "\" \"" VICOS_CXX_STANDARD_LIBRARIES "\"${VICOS_CXX_STANDARD_LIBRARIES}\"")
    set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "${CMAKE_CXX_STANDARD_LIBRARIES_INIT} ${VICOS_CXX_STANDARD_LIBRARIES}")
endif()

# Configuration specific flags.
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
list(APPEND VICOS_LINKER_FLAGS_EXE
	-pie
	-fPIE)
if(VICOS_CPP_FEATURES)
    separate_arguments(VICOS_CPP_FEATURES)
    foreach(feature ${VICOS_CPP_FEATURES})
        if(NOT ${feature} MATCHES "^(rtti|exceptions)$")
            message(FATAL_ERROR "Invalid C++ feature: ${feature}.")
        endif()
        list(APPEND VICOS_COMPILER_FLAGS_CXX -f${feature})
    endforeach()
    string(REPLACE ";" " " VICOS_CPP_FEATURES "${VICOS_CPP_FEATURES}")
endif()
if(NOT VICOS_ALLOW_UNDEFINED_SYMBOLS)
    list(APPEND VICOS_LINKER_FLAGS
		-Wl,--no-undefined)
endif()

# set thumb mode (use -marm for arm mode)
list(APPEND VICOS_COMPILER_FLAGS -mthumb)

list(APPEND VICOS_COMPILER_FLAGS
    -mfpu=neon)
#list(APPEND VICOS_COMPILER_FLAGS
#    -Wa,--noexecstack)
list(APPEND VICOS_LINKER_FLAGS
    -Wl,-z,noexecstack)

# CMake automatically forwards all compiler flags to the linker,
# and clang doesn't like having -Wa flags being used for linking.
# To prevent CMake from doing this would require meddling with
# the CMAKE_<LANG>_COMPILE_OBJECT rules, which would get quite messy.
list(APPEND VICOS_LINKER_FLAGS
    -Qunused-arguments)
list(APPEND VICOS_LINKER_FLAGS
    -Wl,-z,relro -Wl,-z,now)
list(APPEND VICOS_COMPILER_FLAGS
    -Wformat -Werror=format-security)

# Convert these lists into strings.
string(REPLACE ";" " " VICOS_COMPILER_FLAGS         "${VICOS_COMPILER_FLAGS}")
string(REPLACE ";" " " VICOS_COMPILER_FLAGS_CXX     "${VICOS_COMPILER_FLAGS_CXX}")
string(REPLACE ";" " " VICOS_COMPILER_FLAGS_DEBUG   "${VICOS_COMPILER_FLAGS_DEBUG}")
string(REPLACE ";" " " VICOS_COMPILER_FLAGS_RELEASE "${VICOS_COMPILER_FLAGS_RELEASE}")
string(REPLACE ";" " " VICOS_LINKER_FLAGS           "${VICOS_LINKER_FLAGS}")
string(REPLACE ";" " " VICOS_LINKER_FLAGS_EXE       "${VICOS_LINKER_FLAGS_EXE}")

if(VICOS_CCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER   "${VICOS_CCACHE}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${VICOS_CCACHE}")
endif()
set(CMAKE_C_COMPILER        "${VICOS_C_COMPILER}")
set(CMAKE_CXX_COMPILER      "${VICOS_CXX_COMPILER}")
set(_CMAKE_TOOLCHAIN_PREFIX "${VICOS_TOOLCHAIN_PREFIX}")

# Run the compiler ID checks before we set flags.
# When passed the `-march=` flag, Clang can fail to compile if CMake doesn't
# correctly supply the target and external toolchain, but to do so, CMake
# needs to already know that the compiler is Clang. Tell CMake to run the
# compiler ID detection step before we set Android flags.
# See https://gitlab.kitware.com/cmake/cmake/issues/16587
include(CMakeDetermineCCompiler)
include(CMakeDetermineCXXCompiler)

# Set or retrieve the cached flags.
# This is necessary in case the user sets/changes flags in subsequent
# configures. If we included the Android flags in here, they would get
# overwritten.
set(CMAKE_C_FLAGS ""
	CACHE STRING "Flags used by the compiler during all build types.")
set(CMAKE_CXX_FLAGS ""
	CACHE STRING "Flags used by the compiler during all build types.")
set(CMAKE_C_FLAGS_DEBUG ""
	CACHE STRING "Flags used by the compiler during debug builds.")
set(CMAKE_CXX_FLAGS_DEBUG ""
	CACHE STRING "Flags used by the compiler during debug builds.")
set(CMAKE_C_FLAGS_RELEASE ""
	CACHE STRING "Flags used by the compiler during release builds.")
set(CMAKE_CXX_FLAGS_RELEASE ""
	CACHE STRING "Flags used by the compiler during release builds.")
set(CMAKE_MODULE_LINKER_FLAGS ""
	CACHE STRING "Flags used by the linker during the creation of modules.")
set(CMAKE_SHARED_LINKER_FLAGS ""
	CACHE STRING "Flags used by the linker during the creation of dll's.")
set(CMAKE_EXE_LINKER_FLAGS ""
	CACHE STRING "Flags used by the linker.")

set(CMAKE_PREFIX_PATH "${CMAKE_SYSROOT}/usr" CACHE STRING "")
set(CMAKE_SYSTEM_INCLUDE_PATH "${CMAKE_SYSROOT}/usr/include/libunwind" CACHE STRING "")

set(CMAKE_C_FLAGS             "${VICOS_COMPILER_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS           "${VICOS_COMPILER_FLAGS} ${VICOS_COMPILER_FLAGS_CXX} ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG       "${VICOS_COMPILER_FLAGS_DEBUG} ${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_DEBUG     "${VICOS_COMPILER_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE     "${VICOS_COMPILER_FLAGS_RELEASE} ${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_RELEASE   "${VICOS_COMPILER_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_SHARED_LINKER_FLAGS "${VICOS_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${VICOS_LINKER_FLAGS} ${CMAKE_MODULE_LINKER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS    "${VICOS_LINKER_FLAGS} ${VICOS_LINKER_FLAGS_EXE} ${CMAKE_EXE_LINKER_FLAGS}")

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

# <anki>
# Work around missing CMAKE_SIZEOF_VOID_P
#
# Resolves build error
#   FindWinSock.cmake:77 (if):
#   if given arguments:
#     "x86_64" "STREQUAL" "AMD64" "AND" "EQUAL" "4"
# </anki>
set(CMAKE_SIZEOF_VOID_P 4)

# Debug.
message(STATUS "CMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES}")
