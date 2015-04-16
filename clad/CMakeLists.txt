# products-cozmo/game/clad/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

# Since clad command line shows up in comments, make all paths relative
# Dependencies/outputs must be absolute or commands will fail
set(INPUT_DIRECTORY "src")
file(GLOB_RECURSE CLAD_INPUTS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_DIRECTORY}"
    "${INPUT_DIRECTORY}/*.clad")

file(RELATIVE_PATH HPP_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_SOURCE_DIR}/include/anki/cozmo/game")
file(RELATIVE_PATH CPP_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_SOURCE_DIR}/src")
file(RELATIVE_PATH CS_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_SOURCE_DIR}/unity/Cozmo/Assets/Scripts/Channel")

# Every time any clad python file changes, we want to rerun clad
# Make sure this is absolute
set(CLAD_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/anki/cozmo-engine/tools/message-buffers")
file(GLOB CLAD_DEPENDENCIES
    "${CLAD_DIRECTORY}/clad/*.py"
    "${CLAD_DIRECTORY}/emitters/*.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/emitters/*.py")

set(PYTHON python)
set(PYTHONFLAGS -u)

set(CLAD_OUTPUTS "")
foreach(INPUT ${CLAD_INPUTS})
    # Compute current clad INPUT file without extension
    GET_FILENAME_COMPONENT(INPUT_NAME ${INPUT} NAME)
    GET_FILENAME_COMPONENT(INPUT_BASE_NAME ${INPUT} NAME_WE)
    GET_FILENAME_COMPONENT(INPUT_LOCAL_DIRECTORY ${INPUT} DIRECTORY)
    set(INPUT_BASE "${INPUT_LOCAL_DIRECTORY}/${INPUT_BASE_NAME}")
    
    # C++
    # HACK: use .def because xcode doesn't like generated headers
    set(HPP_OUTPUT "${HPP_OUTPUT_DIRECTORY}/${INPUT_BASE}.def")
    set(TAG_HPP_OUTPUT "${HPP_OUTPUT_DIRECTORY}/${INPUT_BASE}Tag.def")
    set(CPP_OUTPUT "${CPP_OUTPUT_DIRECTORY}/${INPUT_BASE}.cpp")
    
    # C#
    set(CS_OUTPUT "${CS_OUTPUT_DIRECTORY}/${INPUT_BASE_NAME}.cs")
    
    set(CLAD_OUTPUTS ${CLAD_OUTPUTS}
        "${CMAKE_CURRENT_SOURCE_DIR}/${HPP_OUTPUT}"
        "${CMAKE_CURRENT_SOURCE_DIR}/${TAG_HPP_OUTPUT}"
        "${CMAKE_CURRENT_SOURCE_DIR}/${CPP_OUTPUT}"
        "${CMAKE_CURRENT_SOURCE_DIR}/${CS_OUTPUT}")
    
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${HPP_OUTPUT}" "${CMAKE_CURRENT_SOURCE_DIR}/${TAG_HPP_OUTPUT}" "${CMAKE_CURRENT_SOURCE_DIR}/${CPP_OUTPUT}"
        
        COMMAND ${PYTHON} ${PYTHONFLAGS} emitters/cozmo_CPP_main_emitter.py
            -C ${INPUT_DIRECTORY}
            -r ${HPP_OUTPUT_DIRECTORY}
            -o ${CPP_OUTPUT_DIRECTORY}
            --header-path-prefix anki/cozmo/game/
            --header-output-extension .def
            ${INPUT}
        
        MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_DIRECTORY}/${INPUT}"
        DEPENDS ${CLAD_DEPENDENCIES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
    )
        
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${CS_OUTPUT}"
        
        COMMAND ${PYTHON} ${PYTHONFLAGS} emitters/cozmo_CSharp_main_emitter.py
            -C ${INPUT_DIRECTORY}
            --output-file-override ${CS_OUTPUT}
            ${INPUT}
        
        MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_DIRECTORY}/${INPUT}"
        DEPENDS ${CLAD_DEPENDENCIES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
    )
    
    # Extra C++ generators for U2G messages only
    if(${INPUT_BASE} MATCHES "U2G")
        
        set(CPP_DECLARATIONS_OUTPUT "${CPP_OUTPUT_DIRECTORY}/${INPUT_BASE}_declarations.def")
        set(CPP_SWITCH_OUTPUT "${CPP_OUTPUT_DIRECTORY}/${INPUT_BASE}_switch.def")
        
        set(CLAD_OUTPUTS ${CLAD_OUTPUTS}
            "${CMAKE_CURRENT_SOURCE_DIR}/${CPP_DECLARATIONS_OUTPUT}"
            "${CMAKE_CURRENT_SOURCE_DIR}/${CPP_SWITCH_OUTPUT}")
        
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${CPP_DECLARATIONS_OUTPUT}"
        
            COMMAND ${PYTHON} ${PYTHONFLAGS} emitters/cozmo_CPP_declarations_emitter.py
                -C ${INPUT_DIRECTORY}
                -o ${CPP_OUTPUT_DIRECTORY}
                ${INPUT}
        
            MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_DIRECTORY}/${INPUT}"
            DEPENDS ${CLAD_DEPENDENCIES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM
        )
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${CPP_SWITCH_OUTPUT}"
        
            COMMAND ${PYTHON} ${PYTHONFLAGS} emitters/cozmo_CPP_switch_emitter.py
                -C ${INPUT_DIRECTORY}
                -o ${CPP_OUTPUT_DIRECTORY}
                ${INPUT}
        
            MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_DIRECTORY}/${INPUT}"
            DEPENDS ${CLAD_DEPENDENCIES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM
        )
    endif()
endforeach()

add_custom_target(
    Cozmo_Clad ALL
    DEPENDS ${CLAD_OUTPUTS}
    VERBATIM
)

