# ccache.cmake
# enable ccache if it is available on the system
# include this file in the toplevel CMakeLists.txt file immediatley after the project() line
#
# source: https://crascit.com/2016/04/09/using-ccache-with-cmake/
#

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    # Set up wrapper scripts
    set(C_LAUNCHER   "${CCACHE_PROGRAM}")
    set(CXX_LAUNCHER "${CCACHE_PROGRAM}")
    configure_file(${CMAKE_SOURCE_DIR}/project/build-scripts/cmake-launch-c.in   launch-c)
    configure_file(${CMAKE_SOURCE_DIR}/project/build-scripts/cmake-launch-cxx.in launch-cxx)
    execute_process(COMMAND chmod a+rx
                     "${CMAKE_BINARY_DIR}/launch-c"
                     "${CMAKE_BINARY_DIR}/launch-cxx"
    )

    if(CMAKE_GENERATOR STREQUAL "Xcode")
        # Set Xcode project attributes to route compilation and linking
        # through our scripts
        set(CMAKE_XCODE_ATTRIBUTE_CC         "${CMAKE_BINARY_DIR}/launch-c")
        set(CMAKE_XCODE_ATTRIBUTE_CXX        "${CMAKE_BINARY_DIR}/launch-cxx")
        set(CMAKE_XCODE_ATTRIBUTE_LD         "${CMAKE_BINARY_DIR}/launch-c")
        set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS "${CMAKE_BINARY_DIR}/launch-cxx")
    else()
        # Support Unix Makefiles and Ninja
        set(CMAKE_C_COMPILER_LAUNCHER   "${CMAKE_BINARY_DIR}/launch-c")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_BINARY_DIR}/launch-cxx")
    endif()
 endif()
