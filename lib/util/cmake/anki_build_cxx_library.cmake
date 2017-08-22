
macro(anki_build_cxx_library target_name srclist_dir)

file(STRINGS "${srclist_dir}/${target_name}.srcs.lst" SRCS)
file(STRINGS "${srclist_dir}/${target_name}.headers.lst" HEADERS)

set(PLATFORM_SRCS "")
set(PLATFORM_HEADERS "")
if (ANDROID)
    file(STRINGS "${srclist_dir}/${target_name}_android.srcs.lst" PLATFORM_SRCS)
    file(STRINGS "${srclist_dir}/${target_name}_android.headers.lst" PLATFORM_HEADERS)
endif()

set(extra_argv ${ARGN})
list(LENGTH extra_argv extra_argc)

if (extra_argc GREATER 0)
  # process extra args here
endif()

add_library(${target_name} ${extra_argv}
  ${SRCS}
  ${HEADERS}
  ${PLATFORM_SRCS}
  ${PLATFORM_HEADERS}
)

endmacro()
