cmake_minimum_required(VERSION 3.6)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 14)
endif()

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

add_subdirectory(tools/message-buffers)
set(CLAD_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tools/message-buffers")
set(CLAD_VICTOR_EMITTER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/victorEmitters")
add_subdirectory(clad)

if(NOT CLAD_OUTPUT_DIR_CPP AND NOT CLAD_OUTPUT_DIR_PYTHON AND NOT CLAD_OUTPUT_DIR_CSHARP AND NOT CLAD_OUTPUT_DIR_C)
    message(FATAL_ERROR "CLAD_OUTPUT_DIR(_CPP|_PYTHON|_CSHARP|_C) is not set. Must provide location to deliver files.")
endif()
