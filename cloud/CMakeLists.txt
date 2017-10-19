cmake_minimum_required(VERSION 3.6)
project(cloud)

include(anki_build_cxx_library)
anki_build_cxx_executable(voicetest ${ANKI_SRCLIST_DIR})

target_link_libraries(voicetest
  PRIVATE
  #util_audio
)
