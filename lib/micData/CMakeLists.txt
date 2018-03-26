cmake_minimum_required(VERSION 3.6)

project(mic_data_proj)

add_library(micdata INTERFACE)

target_include_directories(micdata 
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
