include(anki_build_cxx_compile_definitions)
include(anki_build_cxx_compile_options)
include(anki_build_source_list)
include(anki_build_header_file_only_list)
include(anki_build_data_list)

macro(anki_build_cxx_library target_name srclist_dir)

anki_build_source_list(${target_name} ${srclist_dir})

anki_build_header_file_only_list(${HEADERS})

anki_build_absolute_data_list(${target_name} ${srclist_dir})

set(extra_argv ${ARGN})
list(LENGTH extra_argv extra_argc)

if (extra_argc GREATER 0)
  # process extra args here
endif()
add_library(${target_name} ${extra_argv}
  ${SRCS}
  ${HEADERS}
  ${_ab_PLATFORM_SRCS}
  ${_ab_PLATFORM_HEADERS}
  ${DATA}
)
if(DATA)
  source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${DATA})
endif()

target_compile_definitions(${target_name}
  PRIVATE
  ${ANKI_BUILD_CXX_COMPILE_DEFINITIONS}
)

target_compile_options(${target_name}
  PRIVATE
  ${ANKI_BUILD_CXX_COMPILE_OPTIONS}
)

endmacro()

macro(anki_build_cxx_executable target_name srclist_dir)

anki_build_source_list(${target_name} ${srclist_dir})

anki_build_header_file_only_list(${HEADERS})

anki_build_absolute_data_list(${target_name} ${srclist_dir})

set(extra_argv ${ARGN})
list(LENGTH extra_argv extra_argc)

if (extra_argc GREATER 0)
  # process extra args here
endif()

add_executable(${target_name} ${extra_argv}
  ${SRCS}
  ${HEADERS}
  ${_ab_PLATFORM_SRCS}
  ${_ab_PLATFORM_HEADERS}
  ${DATA}
)

target_compile_definitions(${target_name}
  PRIVATE
  ${ANKI_BUILD_CXX_COMPILE_DEFINITIONS}
)

target_compile_options(${target_name}
  PRIVATE
  ${ANKI_BUILD_CXX_COMPILE_OPTIONS}
)

endmacro()
