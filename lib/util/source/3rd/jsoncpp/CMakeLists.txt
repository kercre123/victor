project(libjsoncpp VERSION 1.8.0 LANGUAGES CXX)

add_library(jsoncpp
  STATIC
  jsoncpp.cpp
)

add_definitions(-DNO_LOCALE_SUPPORT=1)

target_compile_definitions(jsoncpp
  PUBLIC
  JSONCPP_USING_SECURE_MEMORY=0
)

target_include_directories(jsoncpp 
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

