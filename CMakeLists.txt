# coretech-common/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

include(../coretech-common/anki.cmake)

ankiProject(CoreTech_External)

add_subdirectory(gtest-1.7.0)

# Force OpenCV to build dynamic/shared libs:
set(BUILD_SHARED_LIBS "ON")

add_subdirectory(opencv-2.4.6.1)

