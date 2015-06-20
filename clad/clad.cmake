# cozmo-engine/clad/clad.cmake

cmake_minimum_required(VERSION 2.8)

# Cannot reference ${CMAKE_CURRENT_LIST_DIR} within a function
# It is the folder this included file is in.
set(COZMO_ENGINE_CLAD_INCLUDE_LIST_DIR "${CMAKE_CURRENT_LIST_DIR}")

# The directory of the CLAD submodule
# ABSOLUTE also cleans up .. entries
get_filename_component(CLAD_DIR "${COZMO_ENGINE_CLAD_INCLUDE_LIST_DIR}/../tools/anki-util/tools/message-buffers" ABSOLUTE)


# Sets a variable to a relative version of a path or lists of paths after normalizing.
#
# make_relative(VARIABLE_NAME DIRECTORY PATH_LIST)
#
# VARIABLE_NAME: The name of the variable to set.
# DIRECTORY: The directory to make it relative to.
# PATH_LIST: The name of the ${CMAKE_CURRENT_SOURCE_DIR}-relative or absolute path.
#
function(make_relative VARIABLE_NAME DIRECTORY PATH_LIST)
    
    # ABSOLUTE also cleans up .. entries
    get_filename_component(DIRECTORY "${DIRECTORY}" ABSOLUTE)
    
    set(RESULT_LIST "")
    foreach(PATH ${PATH_LIST})
        # ABSOLUTE also cleans up .. entries
        get_filename_component(PATH "${PATH}" ABSOLUTE)
        file(RELATIVE_PATH PATH "${DIRECTORY}" "${PATH}")
        file(TO_CMAKE_PATH "${PATH}" PATH)
        set(RESULT_LIST ${RESULT_LIST} "${PATH}")
    endforeach(PATH)
    
    #message("make_relative ${VARIABLE_NAME}: ${PATH_LIST} -> ${RESULT_LIST}")
    
    # PARENT_SCOPE makes changes the caller's scope
    set("${VARIABLE_NAME}" "${RESULT_LIST}" PARENT_SCOPE)
endfunction(make_relative)


# Sets a variable to the absolute, normalized version of a path or lists of paths.
#
# make_absolute(VARIABLE_NAME PATH_LIST)
#
# VARIABLE_NAME: The name of the variable to set.
# PATH_LIST: The name of the ${CMAKE_CURRENT_SOURCE_DIR}-relative or absolute path.
#
function(make_absolute VARIABLE_NAME PATH_LIST)
    
    set(RESULT_LIST "")
    foreach(PATH ${PATH_LIST})
        # ABSOLUTE also cleans up .. entries
        get_filename_component(PATH "${PATH}" ABSOLUTE)
        set(RESULT_LIST ${RESULT_LIST} "${PATH}")
    endforeach(PATH)
    
    #message("make_absolute ${VARIABLE_NAME}: ${PATH_LIST} -> ${RESULT_LIST}")
    
    # PARENT_SCOPE makes changes the caller's scope
    set("${VARIABLE_NAME}" "${RESULT_LIST}" PARENT_SCOPE)
endfunction(make_absolute)


# Only needed internally.
# Every time any clad python file changes, we want to rerun clad
# Make sure these are absolute
file(GLOB CLAD_DEPENDENCIES
    "${CLAD_DIR}/clad/*.py"
    "${CLAD_DIR}/emitters/*.py"
    "${COZMO_ENGINE_CLAD_INCLUDE_LIST_DIR}/emitters/*.py")
# Detect any change in support libraries. (Ignore hidden files.)
file(GLOB_RECURSE CLAD_SUPPORT_DEPENDENCIES
    "${CLAD_DIR}/support/*.h"
    "${CLAD_DIR}/support/*.c"
    "${CLAD_DIR}/support/*.cpp"
    "${CLAD_DIR}/support/*.cs"
    "${CLAD_DIR}/support/*.py")
set(CLAD_DEPENDENCIES ${CLAD_DEPENDENCIES} ${CLAD_SUPPORT_DEPENDENCIES})
make_absolute("CLAD_DEPENDENCIES" "${CLAD_DEPENDENCIES}")


# Runs CLAD generation for a certain language on a certain file
#
# run_clad(TARGET_NAME EMITTER INPUT_FILE_LIST INPUT_DIRECTORY INCLUDE_DIRECTORY_LIST MAIN_OUTPUT_DIRECTORY HEADER_OUTPUT_DIRECTORY)
#
# GENERATED_FILES_VARIABLE_NAME: The variable name to receive the list of outputs.
# EMITTER: One of C, CPP, cozmo_CPP, CSharp, Python
#     where "cozmo_CPP" will only run both the cozmo_CPP_declarations and cozmo_CPP_switch emitters.
# 
# INPUT_FILE_LIST: A filename or list of filenames to run CLAD on. Must be absolute.
# INPUT_DIRECTORY: The root directory of the input files.
# INCLUDE_DIRECTORY_LIST: A list of input directories 
# 
# MAIN_OUTPUT_DIRECTORY: The folder to save output to.
# HEADER_OUTPUT_DIRECTORY: (C++ only, optional) The folder to save C++ header output to. Defaults to ${MAIN_OUTPUT_DIRECTORY}.
#
function(run_clad GENERATED_FILES_VARIABLE_NAME EMITTER
    INPUT_FILE_LIST INPUT_DIRECTORY INCLUDE_DIRECTORY_LIST
    MAIN_OUTPUT_DIRECTORY)
    if(${ARGC} GREATER 6)
        set(HEADER_OUTPUT_DIRECTORY "${ARGV6}")
    else()
        set(HEADER_OUTPUT_DIRECTORY "${MAIN_OUTPUT_DIRECTORY}")
    endif()
    
    # Input paths needs to be relative to INPUT_DIRECTORY
    get_filename_component(INPUT_DIRECTORY "${INPUT_DIRECTORY}" ABSOLUTE)
    make_relative("INPUT_FILE_LIST" "${INPUT_DIRECTORY}"
        "${INPUT_FILE_LIST}")
    
    # need to make sure we use relative, portable paths when we hit the command line
    # these are relative to ${CMAKE_CURRENT_SOURCE_DIR}
    make_relative("EMITTER_PATH" "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CLAD_DIR}/emitters")
    make_relative("CUSTOM_EMITTER_PATH" "${CMAKE_CURRENT_SOURCE_DIR}"
        "${COZMO_ENGINE_CLAD_INCLUDE_LIST_DIR}/emitters")
    
    # need to make sure we use relative, portable paths when we hit the command line
    # these are relative to ${CMAKE_CURRENT_SOURCE_DIR}
    make_relative("INPUT_DIRECTORY" "${CMAKE_CURRENT_SOURCE_DIR}"
        "${INPUT_DIRECTORY}")
    make_relative("INCLUDE_DIRECTORY_LIST" "${CMAKE_CURRENT_SOURCE_DIR}"
        "${INCLUDE_DIRECTORY_LIST}")
    make_relative("MAIN_OUTPUT_DIRECTORY" "${CMAKE_CURRENT_SOURCE_DIR}"
        "${MAIN_OUTPUT_DIRECTORY}")
    make_relative("HEADER_OUTPUT_DIRECTORY" "${CMAKE_CURRENT_SOURCE_DIR}"
        "${HEADER_OUTPUT_DIRECTORY}")
    
    # Command line, in case parameters need to change
    # -u means don't buffer output (normally you might need to wait a while to see warnings)
    set(PYTHON python)
    set(PYTHONFLAGS -u)
    set(COMMON_COMMAND_LINE -C "${INPUT_DIRECTORY}")
    if(INCLUDE_DIRECTORY_LIST)
        set(COMMON_COMMAND_LINE ${COMMON_COMMAND_LINE} -I "${INCLUDE_DIRECTORY_LIST}")
    endif()
    
    set(ALL_GENERATED_FILES "")
    foreach(INPUT ${INPUT_FILE_LIST})
        # Compute current clad INPUT file without extension
        GET_FILENAME_COMPONENT(INPUT_NAME "${INPUT}" NAME)
        GET_FILENAME_COMPONENT(INPUT_BASE_NAME "${INPUT}" NAME_WE)
        GET_FILENAME_COMPONENT(INPUT_LOCAL_DIRECTORY "${INPUT}" DIRECTORY)
        set(INPUT_BASE "${INPUT_LOCAL_DIRECTORY}/${INPUT_BASE_NAME}")
        
        # Notes about add_custom_command:
        # 1) Make sure COMMANDs use relative paths.
        # 2) Make sure OUTPUTs, MAIN_DEPENDENCY and DEPENDS use absolute paths.
        # 3) Make sure OUTPUTs are each listed separately, not in a list variable.
        
        if("${EMITTER}" STREQUAL "C")
            
            set(H_OUTPUT "${MAIN_OUTPUT_DIRECTORY}/${INPUT_BASE}.h")
            
            set(GENERATED_FILES
                "${CMAKE_CURRENT_SOURCE_DIR}/${H_OUTPUT}")
            make_absolute("GENERATED_FILES" "${GENERATED_FILES}")
            
            add_custom_command(
                OUTPUT ${GENERATED_FILES}
                
                COMMAND "${PYTHON}" "${PYTHONFLAGS}" "${EMITTER_PATH}/C_emitter.py"
                    ${COMMON_COMMAND_LINE}
                    -o "${MAIN_OUTPUT_DIRECTORY}"
                    "${INPUT}"
        
                MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_DIRECTORY}/${INPUT}"
                DEPENDS ${CLAD_DEPENDENCIES}
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                VERBATIM
            )
            
        elseif("${EMITTER}" STREQUAL "CPP")
            
            # HACK: use .def because xcode doesn't like generated headers
            set(HPP_OUTPUT "${HEADER_OUTPUT_DIRECTORY}/${INPUT_BASE}.def")
            set(TAG_HPP_OUTPUT "${HEADER_OUTPUT_DIRECTORY}/${INPUT_BASE}Tag.def")
            set(CPP_OUTPUT "${MAIN_OUTPUT_DIRECTORY}/${INPUT_BASE}.cpp")
            
            set(GENERATED_FILES
                "${CMAKE_CURRENT_SOURCE_DIR}/${HPP_OUTPUT}"
                "${CMAKE_CURRENT_SOURCE_DIR}/${CPP_OUTPUT}")
            make_absolute("GENERATED_FILES" "${GENERATED_FILES}")
            
            add_custom_command(
                OUTPUT ${GENERATED_FILES}
                
                COMMAND "${PYTHON}" "${PYTHONFLAGS}" "${EMITTER_PATH}/CPP_emitter.py"
                    ${COMMON_COMMAND_LINE}
                    -r "${HEADER_OUTPUT_DIRECTORY}"
                    -o "${MAIN_OUTPUT_DIRECTORY}"
                    --header-output-extension .def
                    "${INPUT}"
        
                MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_DIRECTORY}/${INPUT}"
                DEPENDS ${CLAD_DEPENDENCIES}
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                VERBATIM
            )
            
            set(GENERATED_FILES ${GENERATED_FILES}
                "${CMAKE_CURRENT_SOURCE_DIR}/${TAG_HPP_OUTPUT}")
            
        elseif("${EMITTER}" STREQUAL "cozmo_CPP")
            
            # HACK: use .def because xcode doesn't like generated headers
            # These custom emitters are hard-coded to spit out the extension .def
            set(CPP_DECLARATIONS_OUTPUT "${MAIN_OUTPUT_DIRECTORY}/${INPUT_BASE}_declarations.def")
            set(CPP_SWITCH_OUTPUT "${MAIN_OUTPUT_DIRECTORY}/${INPUT_BASE}_switch.def")
        
            set(GENERATED_FILES
                "${CMAKE_CURRENT_SOURCE_DIR}/${CPP_DECLARATIONS_OUTPUT}"
                "${CMAKE_CURRENT_SOURCE_DIR}/${CPP_SWITCH_OUTPUT}")
            make_absolute("GENERATED_FILES" "${GENERATED_FILES}")
            
            add_custom_command(
                OUTPUT ${GENERATED_FILES}
                        
                COMMAND "${PYTHON}" "${PYTHONFLAGS}" "${CUSTOM_EMITTER_PATH}/cozmo_CPP_declarations_emitter.py"
                    ${COMMON_COMMAND_LINE}
                    -o "${MAIN_OUTPUT_DIRECTORY}"
                    "${INPUT}"
        
                COMMAND "${PYTHON}" "${PYTHONFLAGS}" "${CUSTOM_EMITTER_PATH}/cozmo_CPP_switch_emitter.py"
                    ${COMMON_COMMAND_LINE}
                    -o "${MAIN_OUTPUT_DIRECTORY}"
                    "${INPUT}"
        
                MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_DIRECTORY}/${INPUT}"
                DEPENDS ${CLAD_DEPENDENCIES}
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                VERBATIM
            )
                        
        elseif("${EMITTER}" STREQUAL "CSharp")
            
            # C# uses a flattened directory structure
            set(CS_OUTPUT "${MAIN_OUTPUT_DIRECTORY}/${INPUT_BASE_NAME}.cs")
            
            set(GENERATED_FILES
                "${CMAKE_CURRENT_SOURCE_DIR}/${CS_OUTPUT}")
            make_absolute("GENERATED_FILES" "${GENERATED_FILES}")
            
            add_custom_command(
                OUTPUT ${GENERATED_FILES}
                
                COMMAND "${PYTHON}" "${PYTHONFLAGS}" "${EMITTER_PATH}/CSharp_emitter.py"
                    ${COMMON_COMMAND_LINE}
                    --output-file "${CS_OUTPUT}"
                    "${INPUT}"
                
                MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_DIRECTORY}/${INPUT}"
                DEPENDS ${CLAD_DEPENDENCIES}
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                VERBATIM
            )
            
        elseif("${EMITTER}" STREQUAL "Python")
            message(FATAL_ERROR "Python emitter not yet implemented.")
        else()
            message(FATAL_ERROR "Unknown CLAD emitter type \"${EMITTER}\"")
        endif()
        
        set(ALL_GENERATED_FILES ${ALL_GENERATED_FILES} ${GENERATED_FILES})
    endforeach()
    
    set("${GENERATED_FILES_VARIABLE_NAME}" "${ALL_GENERATED_FILES}" PARENT_SCOPE)
endfunction(run_clad)


# Runs run_clad for multiple languages, packing the results into a target.
#
# fully_generate(INPUT_FILE_LIST COZMO_CPP_FILE_LIST INPUT_DIRECTORY INCLUDE_DIRECTORIES C_OUTPUT_DIRECTORY CPP_MAIN_OUTPUT_DIRECTORY CPP_HEADER_OUTPUT_DIRECTORY CSHARP_OUTPUT_DIRECTORY PYTHON_OUTPUT_DIRECTORY)
#
# TARGET_NAME: The name of the custom target to create. Not really needed, but can be used to explicitly build CLAD.
# 
# INPUT_FILE_LIST: The list of CLAD files to process for all emitters.
# COZMO_CPP_FILE_LIST: The list of CLAD files to process for just the custom Cozmo C++ emitters. (Include them in both lists.)
# 
function(create_clad_target TARGET_NAME
    INPUT_FILE_LIST COZMO_CPP_FILE_LIST
    INPUT_DIRECTORY INCLUDE_DIRECTORIES
    C_OUTPUT_DIRECTORY
    CPP_MAIN_OUTPUT_DIRECTORY CPP_HEADER_OUTPUT_DIRECTORY
    CSHARP_OUTPUT_DIRECTORY
    PYTHON_OUTPUT_DIRECTORY)
    
    set(GENERATED_FILES "")
    
    if(C_OUTPUT_DIRECTORY)
        run_clad("CURRENT" "C" "${INPUT_FILE_LIST}" "${INPUT_DIRECTORY}" "${INCLUDE_DIRECTORIES}" "${C_OUTPUT_DIRECTORY}")
        set(GENERATED_FILES ${GENERATED_FILES} ${CURRENT})
    endif()
    
    if(CPP_MAIN_OUTPUT_DIRECTORY)
        run_clad("CURRENT" "CPP" "${INPUT_FILE_LIST}" "${INPUT_DIRECTORY}" "${INCLUDE_DIRECTORIES}" "${CPP_MAIN_OUTPUT_DIRECTORY}" "${CPP_HEADER_OUTPUT_DIRECTORY}")
        set(GENERATED_FILES ${GENERATED_FILES} ${CURRENT})
    endif()
    
    if(COZMO_CPP_FILE_LIST AND CPP_MAIN_OUTPUT_DIRECTORY)
        run_clad("CURRENT" "cozmo_CPP" "${COZMO_CPP_FILE_LIST}" "${INPUT_DIRECTORY}" "${INCLUDE_DIRECTORIES}" "${CPP_MAIN_OUTPUT_DIRECTORY}")
        set(GENERATED_FILES ${GENERATED_FILES} ${CURRENT})
    endif()
    
    if(CSHARP_OUTPUT_DIRECTORY)
        run_clad("CURRENT" "CSharp" "${INPUT_FILE_LIST}" "${INPUT_DIRECTORY}" "${INCLUDE_DIRECTORIES}" "${CSHARP_OUTPUT_DIRECTORY}")
        set(GENERATED_FILES ${GENERATED_FILES} ${CURRENT})
    endif()
    
    if(PYTHON_OUTPUT_DIRECTORY)
        run_clad("CURRENT" "Python" "${INPUT_FILE_LIST}" "${INPUT_DIRECTORY}" "${INCLUDE_DIRECTORIES}" "${PYTHON_OUTPUT_DIRECTORY}")
        set(GENERATED_FILES ${GENERATED_FILES} ${CURRENT})
    endif()
    
    #message("GENERATED_FILES: ${GENERATED_FILES}")
    
    if(GENERATED_FILES)
        add_custom_target(
            "${TARGET_NAME}" ALL
            DEPENDS ${GENERATED_FILES}
            VERBATIM
        )
    else()
        message(FATAL_ERROR "create_clad_target: Nothing to do.")
    endif()
endfunction(create_clad_target)
