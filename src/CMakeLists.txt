# products-cozmo/game/src/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

add_definitions(-DCOZMO_BASESTATION)
add_definitions(-DCORETECH_BASESTATION)

set(CLAD_DIR "${CMAKE_SOURCE_DIR}/lib/anki/cozmo-engine/tools/message-buffers")

file(GLOB CLAD_SOURCES "*.clad")

file(GLOB CLAD_DEPENDENCIES "${CLAD_DIR}/parser/*.py" "${CLAD_DIR}/emitters/*.py")
set(CLAD_BUILD_DIR "${CMAKE_BINARY_DIR}/clad")

set(CLAD_CPP_OUTPUTS)
set(CLAD_CS_OUTPUTS)
foreach(SOURCE ${CLAD_SOURCES})
    GET_FILENAME_COMPONENT(SOURCE_BASE ${SOURCE} NAME_WE)
    set(CPP_BUILD "${CLAD_BUILD_DIR}/${SOURCE_BASE}.def")
    set(CPP_OUTPUT "${CMAKE_SOURCE_DIR}/include/anki/cozmo/game/comms/messaging/${SOURCE_BASE}.def")
    set(CS_BUILD "${CLAD_BUILD_DIR}/${SOURCE_BASE}.cs")
    set(CS_OUTPUT "${CMAKE_SOURCE_DIR}/unity/Cozmo/Assets/Scripts/Channel/${SOURCE_BASE}.cs")
    set(CLAD_COMMANDS ${CLAD_COMMANDS} ${CPP_BUILD} ${CS_BUILD})
    add_custom_command(
        OUTPUT ${CPP_BUILD}
        COMMAND mkdir -p ${CLAD_BUILD_DIR}
        COMMAND python -u "${CLAD_DIR}/emitters/CPP_emitter.py" -n Anki -n Cozmo ${SOURCE} > ${CPP_BUILD}
        COMMAND diff ${CPP_BUILD} ${CPP_OUTPUT} > /dev/null || cp ${CPP_BUILD} ${CPP_OUTPUT} && touch ${CPP_OUTPUT}
        MAIN_DEPENDENCY ${SOURCE}
        DEPENDS ${CLAD_DEPENDENCIES}
        VERBATIM
    )
    add_custom_command(
        OUTPUT ${CS_BUILD}
        COMMAND mkdir -p ${CLAD_BUILD_DIR}
        COMMAND python -u "${CLAD_DIR}/emitters/CSharp_emitter.py" -n Anki.Cozmo ${SOURCE} > ${CS_BUILD}
        COMMAND diff ${CS_BUILD} ${CS_OUTPUT} > /dev/null || cp ${CS_BUILD} ${CS_OUTPUT} && touch ${CS_OUTPUT}
        MAIN_DEPENDENCY ${SOURCE}
        DEPENDS ${CLAD_DEPENDENCIES}
        VERBATIM
    )
endforeach()

#set(SAFE_MESSAGE_BUFFER_FILES "${CLAD_DIR}/emitters/SafeMessageBuffer.h" "${CLAD_DIR}/emitters/SafeMessageBuffer.cpp")
#foreach(SOURCE ${SAFE_MESSAGE_BUFFER_FILES})
#    set(OUTPUT "${CMAKE_SOURCE_DIR}/include/anki/cozmo/game/comms/messaging/${SOURCE}")
#    set(CLAD_COMMAND "${SOURCE}.xxx")
#    set(CLAD_COMMANDS ${CLAD_COMMANDS} ${CLAD_COMMAND})
#    add_custom_command(
#        OUTPUT ${CLAD_COMMAND}
#        COMMAND diff ${SOURCE} ${OUTPUT} > /dev/null || cp ${SOURCE} ${OUTPUT}
#        MAIN_DEPENDENCY ${SOURCE}
#        VERBATIM
#    )
#endforeach()

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
file(GLOB HEADER_FILES_COMMS_MESSAGING "comms/messaging/*")

set(SOURCE_FILES_CLAD "${CLAD_DIR}/emitters/SafeMessageBuffer.cpp")
set(HEADER_FILES_CLAD "${CLAD_DIR}/emitters/SafeMessageBuffer.h")

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
  ${PUBLIC_HEADER_FILES_COMMS_MESSAGING}
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




