# cozmo-game/game/clad/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

# NEW FILES

file(GLOB_RECURSE CLAD_GAME_INPUT_FILE_LIST
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.clad")

file(GLOB_RECURSE CLAD_GAME_COZMO_CPP_INPUT_FILE_LIST
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*U2G*.clad")
    
create_clad_target("Cozmo_Game_Clad"
    # Input files for all, specialized C++
    "${CLAD_GAME_INPUT_FILE_LIST}" "${CLAD_GAME_COZMO_CPP_INPUT_FILE_LIST}"
    # Base CLAD source path, extra included directories
    "src" "${COZMO_ENGINE_DIR}/clad/src/"
    # C output path
    ""
    # C++ output path (source, headers)
    "${PROJECT_SOURCE_DIR}/game/src" "${PROJECT_SOURCE_DIR}/game/include"
    # C# output path (flat)
    "${PROJECT_SOURCE_DIR}/unity/Cozmo/Assets/Scripts/MessageBuffers"
    # Python output path
    "")

# ENGINE FILES (generate C# only)

# keep in sync with other versions in cozmo-engine
file(GLOB_RECURSE CLAD_ENGINE_INPUT_FILE_LIST
    "${COZMO_ENGINE_DIR}/clad/src/*.clad")
    
create_clad_target("Cozmo_Engine_Clad_CS"
    # Input files for all, specialized C++
    "${CLAD_ENGINE_INPUT_FILE_LIST}" ""
    # Base CLAD source path, extra included directories
    "${COZMO_ENGINE_DIR}/clad/src" ""
    # C output path
    ""
    # C++ output path (source, headers)
    "" ""
    # C# output path (flat)
    "${PROJECT_SOURCE_DIR}/unity/Cozmo/Assets/Scripts/MessageBuffers"
    # Python output path
    "")
