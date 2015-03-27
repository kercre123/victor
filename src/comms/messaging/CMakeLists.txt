# products-cozmo/game/src/comms/messaging/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

file(RELATIVE_PATH RELATIVE_CLAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CLAD_DIR}")

file(GLOB CLAD_SOURCES
    "*.clad")

file(GLOB CLAD_DEPENDENCIES
    "${CLAD_DIR}/clad/*.py"
    "${CLAD_DIR}/emitters/*.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/comms/messaging/*.py")

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
        COMMAND python -u "${RELATIVE_CLAD_DIR}/emitters/CPP_emitter.py" -o ${CPP_OUTPUT} -r ${HPP_OUTPUT} ${RELATIVE_SOURCE}
        MAIN_DEPENDENCY ${SOURCE}
        DEPENDS ${CLAD_DEPENDENCIES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        VERBATIM
    )
    add_custom_command(
        OUTPUT ${CS_OUTPUT}
        COMMAND python -u "${RELATIVE_CLAD_DIR}/emitters/CSharp_emitter.py" -o ${CS_OUTPUT} ${RELATIVE_SOURCE}
        MAIN_DEPENDENCY ${SOURCE}
        DEPENDS ${CLAD_DEPENDENCIES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        VERBATIM
    )
    
    if(${SOURCE_BASE} MATCHES "U2G")
        file(RELATIVE_PATH CPP_DECLARATIONS "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_SOURCE_DIR}/src/comms/messaging/${SOURCE_BASE}_declarations.def")
        file(RELATIVE_PATH CPP_SWITCH "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_SOURCE_DIR}/src/comms/messaging/${SOURCE_BASE}_switch.def")
        set(CLAD_COMMANDS ${CLAD_COMMANDS} ${CPP_DECLARATIONS} ${CPP_SWITCH})
        
        add_custom_command(
            OUTPUT ${CPP_DECLARATIONS}
            COMMAND python -u "cozmo_CPP_declarations_emitter.py" -o ${CPP_DECLARATIONS} ${SOURCE}
            MAIN_DEPENDENCY ${SOURCE}
            DEPENDS ${CLAD_DEPENDENCIES}
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            VERBATIM
        )
        add_custom_command(
            OUTPUT ${CPP_SWITCH}
            COMMAND python -u "cozmo_CPP_switch_emitter.py" -o ${CPP_SWITCH} ${SOURCE}
            MAIN_DEPENDENCY ${SOURCE}
            DEPENDS ${CLAD_DEPENDENCIES}
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            VERBATIM
        )
    endif()
endforeach()

add_custom_target(
    CLAD ALL
    DEPENDS ${CLAD_COMMANDS}
    VERBATIM
)




