cmake_minimum_required(VERSION 2.8)

set(CMAKE_PREFIX_PATH "../../../share")

find_package(royale REQUIRED)

link_directories(${royale_LIB_DIR})

project (sampleExportPLY)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../inc
)

add_executable(sampleExportPLY
   sampleExportPLY.cpp
   )

COPY_ROYALE_LIBS(sampleExportPLY)

target_link_libraries(sampleExportPLY "${royale_LIBS}")
