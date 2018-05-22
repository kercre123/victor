cmake_minimum_required(VERSION 3.6)

project(noise)

include(anki_build_cxx_compile_options)
include(anki_build_cxx_compile_definitions)
include(anki_build_strip)

find_package(ALSA REQUIRED)

if(NOT ALSA_FOUND)
    message(ERROR "ALSA NOT FOUND!!!")
else()
    message(INFO "ALSA FOUND!!!")
endif()

# ---------------------------------------------------

add_executable(noise
    pcm_min.c
)

target_link_libraries(noise
  ${ALSA_LIBRARY}
  ${PLATFORM_LIBS}
)

anki_build_strip(TARGET noise)

target_compile_definitions(noise
  PRIVATE
  ${ANKI_BUILD_CXX_COMPILE_DEFINITIONS}
)

target_compile_options(noise
  PRIVATE
  ${ANKI_BUILD_CXX_COMPILE_OPTIONS}
)

# ---------------------------------------------------

add_executable(pcm
    pcm.c
)

target_link_libraries(pcm
  ${ALSA_LIBRARY}
  ${PLATFORM_LIBS}
)

anki_build_strip(TARGET pcm)

target_compile_definitions(pcm
  PRIVATE
  ${ANKI_BUILD_CXX_COMPILE_DEFINITIONS}
)

target_compile_options(pcm
  PRIVATE
  ${ANKI_BUILD_CXX_COMPILE_OPTIONS}
)
