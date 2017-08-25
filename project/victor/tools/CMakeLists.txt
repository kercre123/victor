cmake_minimum_required(VERSION 3.6)
project(victor_tools)

include(android_strip)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
set(PLATFORM_FLAGS "")
if (ANDROID)
  set(PLATFORM_LIBS log)
elseif (MACOSX)
  find_library(FOUNDATION Foundation)
  set(PLATFORM_LIBS
    ${FOUNDATION}
  )
  set(PLATFORM_FLAGS -fobjc-arc)
endif()

add_executable(axattr
    axattr/axattr.c
)

target_compile_options(axattr
PRIVATE
  -DNDEBUG
  -Os
)

android_strip(TARGET axattr)
