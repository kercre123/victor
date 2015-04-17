# products-cozmo/game/src/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

add_definitions(-DCOZMO_BASESTATION)
add_definitions(-DCORETECH_BASESTATION)

file(GLOB SOURCE_FILES
    "src/anki/cozmo/game/*.cpp"
    "src/anki/cozmo/game/comms/*.cpp"
    "src/anki/cozmo/game/comms/messaging/*.cpp"
    "src/anki/cozmo/game/signals/*.cpp"
    "${CLAD_DIR}/support/cpp/source/*.cpp"
)

file(GLOB PRIVATE_HEADER_FILES
    "src/anki/cozmo/game/*.h"
    "src/anki/cozmo/game/comms/*.h"
    "src/anki/cozmo/game/comms/messaging/*.h"
    "src/anki/cozmo/game/comms/messaging/*.def"
    "src/anki/cozmo/game/signals/*.h"
    "src/anki/cozmo/game/signals/*.def"
    "${CLAD_DIR}/support/cpp/include/CLAD/*.h"
)

file(GLOB PUBLIC_HEADER_FILES
    "include/anki/cozmo/game/*.h"
    "include/anki/cozmo/game/comms/*.h"
    "include/anki/cozmo/game/comms/messaging/*.h"
    "include/anki/cozmo/game/comms/messaging/*.def"
    "include/anki/cozmo/game/signals/*.h"
    "include/anki/cozmo/game/signals/*.def")

include_directories("src")

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

add_dependencies(Cozmo_Game Cozmo_Clad)




