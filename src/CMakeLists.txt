# products-cozmo/game/src/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

add_definitions(-DCOZMO_BASESTATION)
add_definitions(-DCORETECH_BASESTATION)


add_subdirectory("comms/messaging")


set(CLAD_DIR "${CMAKE_SOURCE_DIR}/lib/anki/cozmo-engine/tools/message-buffers")

file(GLOB SOURCE_FILES
    "*.cpp"
    "signals/*.cpp"
    "comms/*.cpp"
    "comms/messaging/*.cpp"
    "${CLAD_DIR}/support/cpp/source/*.cpp"
)

file(GLOB PRIVATE_HEADER_FILES
    "*.h"
    "signals/*.h"
    "comms/*.h"
    "comms/messaging/*.h"
    "comms/messaging/*.def"
    "${CLAD_DIR}/support/cpp/include/CLAD/*.h"
)

file(GLOB PUBLIC_HEADER_FILES
    "${COZMO_GAME_INCLUDE_DIR}/anki/cozmo/game/*.h"
    "${COZMO_GAME_INCLUDE_DIR}/anki/cozmo/game/signals/*.h"
    "${COZMO_GAME_INCLUDE_DIR}/anki/cozmo/game/signals/*.def"
    "${COZMO_GAME_INCLUDE_DIR}/anki/cozmo/game/comms/*.h"
    "${COZMO_GAME_INCLUDE_DIR}/anki/cozmo/game/comms/messaging/*.h"
    "${COZMO_GAME_INCLUDE_DIR}/anki/cozmo/game/comms/messaging/*.def")

# Enable ARC
#add_compile_options(-fobjc-arc)

add_library(Cozmo_Game
  ${SOURCE_FILES} 
  ${PRIVATE_HEADER_FILES}
  ${PUBLIC_HEADER_FILES}
)

target_link_libraries(Cozmo_Game
  Cozmo_Basestation 
  CoreTech_Messaging_Basestation
  CoreTech_Common_Basestation 
)

add_dependencies(Cozmo_Game CLAD)




