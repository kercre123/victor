# products-cozmo/game/src/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

add_definitions(-DCOZMO_BASESTATION)
add_definitions(-DCORETECH_BASESTATION)

file(GLOB_RECURSE SOURCE_FILES
    "src/*.cpp"
)

file(GLOB_RECURSE PRIVATE_HEADER_FILES
    "src/*.h"
    "src/*.def"
)

file(GLOB_RECURSE PUBLIC_HEADER_FILES
    "include/*.h"
    "include/*.def")

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
  Clad_Support_Cpp
)

add_dependencies(Cozmo_Game
    Cozmo_Game_Clad)
