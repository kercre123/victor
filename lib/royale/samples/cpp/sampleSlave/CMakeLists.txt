cmake_minimum_required(VERSION 2.8)

set(CMAKE_PREFIX_PATH "../../../share")

find_package(royale REQUIRED)

link_directories(${royale_LIB_DIR})

project (sampleSlave)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../inc
)

add_executable(sampleSlave
   sampleSlave.cpp
   )

COPY_ROYALE_LIBS(sampleSlave)

target_link_libraries(sampleSlave "${royale_LIBS}")
