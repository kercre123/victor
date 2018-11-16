cmake_minimum_required(VERSION 3.6)

project(ion-sample)

include(anki_build_cxx)

if (VICOS AND ANKI_BUILD_OPENCL_ION_MEM_EXAMPLE)

set( ION_MEM_MAP_SRCS
     osal.c clstlib.c ion_mem_map.c ion_mem.c
   )

add_executable( ion_mem_map ${ION_MEM_MAP_SRCS} )
target_include_directories( ion_mem_map 
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  SYSTEM PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_definitions( ion_mem_map 
  PRIVATE
  USE_ION
  USE_ANDROID_LOGGING=1
)

target_link_libraries( ion_mem_map
    OpenCL
)
set_target_properties(ion_mem_map PROPERTIES LINKER_LANGUAGE C)
anki_build_target_license(ion_mem_map "ANKI")

endif()

