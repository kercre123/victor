# products-cozmo/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

include(../coretech-common/anki.cmake)

ankiProject(Cozmo)

add_subdirectory(src) 
add_subdirectory(test)
#add_subdirectory(matlab/mex)
