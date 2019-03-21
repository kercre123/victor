cmake_minimum_required(VERSION 2.8)

set(CMAKE_PREFIX_PATH "../../../share")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../share")

# This needs OpenNI2
find_package(OpenNI2)

if(NOT OpenNI2_FOUND)
    message(WARNING "OpenNI2Driver example will not be build as no OpenNI2 was found!")
    return()
endif()

find_package(royale REQUIRED)

link_directories(${royale_LIB_DIR})

project (sampleOpenNI2Driver)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../inc
)

include_directories(
    SYSTEM
    ${OPENNI2_INCLUDE_DIR}
)

add_library(
    royaleONI2 SHARED
    RoyaleOpenNI2Driver.cpp
    RoyaleOpenNI2Device.cpp
    RoyaleOpenNI2DepthStream.cpp
    RoyaleOpenNI2Driver.hpp
    RoyaleOpenNI2Device.hpp
    RoyaleOpenNI2DepthStream.hpp
    )

COPY_ROYALE_LIBS(royaleONI2)

target_link_libraries(
    royaleONI2
    royale ${OPENNI2_LIBRARY}
    )

set_target_properties(royaleONI2
    PROPERTIES
    OUTPUT_NAME royaleONI2
    FOLDER modules
    VERSION 0
    )
