cmake_minimum_required(VERSION 2.8)

# Please insert your OpenCV path
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} [path to folder where you put FindOpenCV.cmake])

# This needs at least OpenCV 2.x, but it should also work with OpenCV 3.x
find_package(OpenCV QUIET)

if(NOT OpenCV_FOUND)
  message("OpenCV example will not be build as no OpenCV was found!")
  return()
endif()

set_target_properties(${OpenCV_LIBS} PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)
set_target_properties(${OpenCV_LIBS} PROPERTIES MAP_IMPORTED_CONFIG_MINSIZEREL RELEASE)

set(CMAKE_PREFIX_PATH "../../../share")

find_package(royale REQUIRED)

link_directories(${royale_LIB_DIR})

project (sampleOpenCV)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../inc
)

add_executable(sampleOpenCV
   sampleOpenCV.cpp
   )

if(WIN32)   
    if(OpenCV_STATIC)
        target_compile_options(sampleOpenCV PRIVATE "/MT$<$<CONFIG:Debug>:d>")
    endif()
endif()
   
COPY_ROYALE_LIBS(sampleOpenCV)

target_link_libraries(sampleOpenCV "${royale_LIBS}" "${OpenCV_LIBS}")
