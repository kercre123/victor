# products-cozmo/game/clad/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

file(GLOB_RECURSE CLAD_INPUT_FILE_LIST
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.clad")

file(GLOB_RECURSE CLAD_COZMO_CPP_INPUT_FILE_LIST
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*U2G*.clad")
    
create_clad_target("Cozmo_Game_Clad"
    "${CLAD_INPUT_FILE_LIST}" "${CLAD_COZMO_CPP_INPUT_FILE_LIST}"
    "src" "${COZMO_ENGINE_DIR}/clad/src/"
    ""
    "${PROJECT_SOURCE_DIR}/game/src" "${PROJECT_SOURCE_DIR}/game/include"
    "${PROJECT_SOURCE_DIR}/unity/Cozmo/Assets/Scripts/MessageBuffers"
    "")
