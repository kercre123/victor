# products-cozmo/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

include(../coretech-common/anki.cmake)

ankiProject(Cozmo)

include_directories(${PROJECT_SOURCE_DIR}/../coretech-vision/include)
link_directories(${PROJECT_SOURCE_DIR}/../coretech-vision/build/${CMAKE_GENERATOR}/lib/${BUILD_TYPE_DIR})

add_subdirectory(src) 
add_subdirectory(test)
#add_subdirectory(matlab/mex)
