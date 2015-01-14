# products-cozmo/game/src/CMakeLists.txt

# root src dir
file(GLOB SOURCE_FILES_ROOT "*.cpp")
file(GLOB HEADER_FILES_ROOT "*.h")

# signals src dir
file(GLOB SOURCE_FILES_SIGNALS "signals/*.cpp")
file(GLOB HEADER_FILES_SIGNALS "signals/*.h")

set(PRIVATE_HEADER_FILES
  ${HEADER_FILES_ROOT}
  ${HEADER_FILES_SIGNALS}
)

set(SOURCE_FILES
  ${SOURCE_FILES_ROOT}
  ${SOURCE_FILES_SIGNALS}
)

# Enable ARC
add_compile_options(-fobjc-arc)

message(STATUS "COZMO_GAME_INCLUDE_DIR = ${COZMO_GAME_INCLUDE_DIR}")

file(GLOB PUBLIC_HEADER_FILES_ROOT "${COZMO_GAME_INCLUDE_DIR}/anki/cozmo/game/*.h")
file(GLOB PUBLIC_HEADER_FILES_SIGNALS "${COZMO_GAME_INCLUDE_DIR}/anki/cozmo/game/signals/*.h")

set(PUBLIC_HEADER_FILES
  ${PUBLIC_HEADER_FILES_ROOT}
  ${PUBLIC_HEADER_FILES_SIGNALS}
)

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

