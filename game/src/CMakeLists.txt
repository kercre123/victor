# products-cozmo/game/src/CMakeLists.txt

# root src dir
file(GLOB SOURCE_FILES "*.cpp")
file(GLOB PRIVATE_HEADER_FILES "*.h")

# Enable ARC
add_compile_options(-fobjc-arc)

message(STATUS "COZMO_GAME_INCLUDE_DIR = ${COZMO_GAME_INCLUDE_DIR}")

file(GLOB PUBLIC_HEADER_FILES "${COZMO_GAME_INCLUDE_DIR}/anki/cozmo/game/*.h")

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

