# products-cozmo/game/src/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

add_definitions(-DCOZMO_BASESTATION)
add_definitions(-DCORETECH_BASESTATION)

file(RELATIVE_PATH CLAD_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/lib/anki/cozmo-engine/tools/message-buffers")

file(GLOB_RECURSE CLAD_SOURCES "*.clad")

file(GLOB CLAD_DEPENDENCIES
    "${CLAD_DIR}/clad/*.py"
    "${CLAD_DIR}/emitters/*.py"
    "comms/messaging/*.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt")

foreach(SOURCE ${CLAD_SOURCES})
    file(RELATIVE_PATH RELATIVE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${SOURCE}")
    
    GET_FILENAME_COMPONENT(SOURCE_BASE ${SOURCE} NAME_WE)
    file(RELATIVE_PATH CPP_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/src/comms/messaging/${SOURCE_BASE}.cpp")
    file(RELATIVE_PATH HPP_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/include/anki/cozmo/game/comms/messaging/${SOURCE_BASE}.def")
    file(RELATIVE_PATH CS_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/unity/Cozmo/Assets/Scripts/Channel/${SOURCE_BASE}.cs")
    set(CLAD_COMMANDS ${CLAD_COMMANDS} ${CPP_OUTPUT} ${HPP_OUTPUT} ${CS_OUTPUT})
    
    add_custom_command(
        OUTPUT ${CPP_OUTPUT} ${HPP_OUTPUT}
        COMMAND python -u "${CLAD_DIR}/emitters/CPP_emitter.py" -o ${CPP_OUTPUT} -r ${HPP_OUTPUT} ${RELATIVE_SOURCE}
        MAIN_DEPENDENCY ${SOURCE}
        DEPENDS ${CLAD_DEPENDENCIES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        VERBATIM
    )
    add_custom_command(
        OUTPUT ${CS_OUTPUT}
        COMMAND python -u "${CLAD_DIR}/emitters/CSharp_emitter.py" -o ${CS_OUTPUT} ${RELATIVE_SOURCE}
        MAIN_DEPENDENCY ${SOURCE}
        DEPENDS ${CLAD_DEPENDENCIES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        VERBATIM
    )
endforeach()

#include_directories(
#    "${CLAD_DIR}/emitters")

add_custom_target(
    CLAD ALL
    DEPENDS ${CLAD_COMMANDS}
    VERBATIM
)

# root src dir
file(GLOB SOURCE_FILES_ROOT "*.cpp")
file(GLOB HEADER_FILES_ROOT "*.h")

# signals src dir
file(GLOB SOURCE_FILES_SIGNALS "signals/*.cpp")
file(GLOB HEADER_FILES_SIGNALS "signals/*.h")

# comms src dir
file(GLOB SOURCE_FILES_COMMS "comms/*.cpp")
file(GLOB HEADER_FILES_COMMS "comms/*.h")

# comms/messaging src dir
file(GLOB SOURCE_FILES_COMMS_MESSAGING "comms/messaging/*.cpp")
file(GLOB HEADER_FILES_COMMS_MESSAGING_H "comms/messaging/*.h")
file(GLOB HEADER_FILES_COMMS_MESSAGING_DEF "comms/messaging/*.def")

file(GLOB_RECURSE SOURCE_FILES_CLAD "${CLAD_DIR}/support/cpp/*.cpp")
file(GLOB_RECURSE HEADER_FILES_CLAD "${CLAD_DIR}/support/cpp/*.h")

set(PRIVATE_HEADER_FILES
  ${HEADER_FILES_ROOT}
  ${HEADER_FILES_SIGNALS}
  ${HEADER_FILES_COMMS}
  ${HEADER_FILES_COMMS_MESSAGING}
  ${HEADER_FILES_CLAD}
)

set(SOURCE_FILES
  ${SOURCE_FILES_ROOT}
  ${SOURCE_FILES_SIGNALS}
  ${SOURCE_FILES_COMMS}
  ${SOURCE_FILES_COMMS_MESSAGING}
  ${SOURCE_FILES_CLAD}
)

# Enable ARC
#add_compile_options(-fobjc-arc)

message(STATUS "COZMO_GAME_INCLUDE_DIR = ${COZMO_GAME_INCLUDE_DIR}")

file(GLOB PUBLIC_HEADER_FILES_ROOT "${COZMO_GAME_INCLUDE_DIR}/anki/cozmo/game/*.h")
file(GLOB PUBLIC_HEADER_FILES_SIGNALS "${COZMO_GAME_INCLUDE_DIR}/anki/cozmo/game/signals/*")
file(GLOB PUBLIC_HEADER_FILES_COMMS "${COZMO_GAME_INCLUDE_DIR}/anki/cozmo/game/comms/*.h")
file(GLOB PUBLIC_HEADER_FILES_COMMS_MESSAGING "${COZMO_GAME_INCLUDE_DIR}/anki/cozmo/game/comms/messaging/*.h")

set(PUBLIC_HEADER_FILES
  ${PUBLIC_HEADER_FILES_ROOT}
  ${PUBLIC_HEADER_FILES_SIGNALS}
  ${PUBLIC_HEADER_FILES_COMMS}
  ${PUBLIC_HEADER_FILES_COMMS_MESSAGING_H}
  ${PUBLIC_HEADER_FILES_COMMS_MESSAGING_DEF}
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

add_dependencies(Cozmo_Game CLAD)




