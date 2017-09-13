
macro(__anki_build_cxx_source_list target_name srclist_dir)
file(STRINGS "${srclist_dir}/${target_name}.srcs.lst" SRCS)
file(STRINGS "${srclist_dir}/${target_name}.headers.lst" HEADERS)

set(_abcxx_PLATFORM_SRCS "")
set(_abcxx_PLATFORM_HEADERS "")
if (ANDROID)
    if (EXISTS "${srclist_dir}/${target_name}_android.srcs.lst")
      file(STRINGS "${srclist_dir}/${target_name}_android.srcs.lst" _abcxx_PLATFORM_SRCS)
    endif()
    if (EXISTS "${srclist_dir}/${target_name}_android.headers.lst")
      file(STRINGS "${srclist_dir}/${target_name}_android.headers.lst" _abcxx_PLATFORM_HEADERS)
    endif()
elseif (MACOSX)
    if (EXISTS "${srclist_dir}/${target_name}_mac.srcs.lst")
      file(STRINGS "${srclist_dir}/${target_name}_mac.srcs.lst" _abcxx_PLATFORM_SRCS)
    endif()
    if (EXISTS "${srclist_dir}/${target_name}_mac.headers.lst")
      file(STRINGS "${srclist_dir}/${target_name}_mac.headers.lst" _abcxx_PLATFORM_HEADERS)
    endif()
endif()

endmacro()

macro(anki_build_cxx_library target_name srclist_dir)
__anki_build_cxx_source_list(${target_name} ${srclist_dir})

set(extra_argv ${ARGN})
list(LENGTH extra_argv extra_argc)

if (extra_argc GREATER 0)
  # process extra args here
endif()

add_library(${target_name} ${extra_argv}
  ${SRCS}
  ${HEADERS}
  ${_abcxx_PLATFORM_SRCS}
  ${_abcxx_PLATFORM_HEADERS}
)
endmacro()

macro(anki_build_cxx_executable target_name srclist_dir)
__anki_build_cxx_source_list(${target_name} ${srclist_dir})

set(extra_argv ${ARGN})
list(LENGTH extra_argv extra_argc)

if (extra_argc GREATER 0)
  # process extra args here
endif()

add_executable(${target_name} ${extra_argv}
  ${SRCS}
  ${HEADERS}
  ${_abcxx_PLATFORM_SRCS}
  ${_abcxx_PLATFORM_HEADERS}
)
endmacro()
