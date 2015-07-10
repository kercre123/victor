# products-cozmo/game/clad/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

# keep in sync with C# version in cozmo-game
file(GLOB_RECURSE CLAD_ENGINE_INPUT_FILE_LIST
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.clad")

file(GLOB_RECURSE CLAD_ENGINE_COZMO_CPP_INPUT_FILE_LIST
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*R2B*.clad")


create_clad_target("Cozmo_Engine_Clad"
    # Input files for all, specialized C++
    "${CLAD_ENGINE_INPUT_FILE_LIST}" "${CLAD_ENGINE_COZMO_CPP_INPUT_FILE_LIST}"
    # Base CLAD source path, extra included directories
    "src" ""
    # C output path
    #"${PROJECT_SOURCE_DIR}/robot/include" -- Don't need C emitter run on this stuff?
    ""
    # C++ output path (source, headers)
    "${PROJECT_SOURCE_DIR}/basestation/src" "${PROJECT_SOURCE_DIR}/basestation/include"
    # C# output path (flat)
    ""
    # Python output path
    "")

file(GLOB CLAD_SUPPORT_CPP
    "${CLAD_DIR}/support/cpp/include/CLAD/*.h"
    "${CLAD_DIR}/support/cpp/source/*.cpp")
add_library(
    Clad_Support_Cpp STATIC
    ${CLAD_SUPPORT_CPP})
