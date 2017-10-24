cmake_minimum_required(VERSION 3.6)
project(cloud)

include(anki_build_cxx)
include(anki_build_go)

# voicetest project (mac only)
if (MACOSX)

  # create Go target
  anki_build_go_c_library(voicego VOICEGO_HEADER "voicetest/go" "go" ${ANKI_SRCLIST_DIR})
  get_filename_component(VOICEGO_HEADER_DIR ${VOICEGO_HEADER} DIRECTORY)

  # create C target
  anki_build_cxx_executable(voicetest ${ANKI_SRCLIST_DIR} ${VOICEGO_HEADER})

  # tell voicetest where to find voicego.h
  target_include_directories(voicetest
    PUBLIC
    ${VOICEGO_HEADER_DIR}
  )

  # link audio and go libraries
  target_link_libraries(voicetest
    PRIVATE
    util_audio
    voicego
  )
endif()

# ipctest - for testing interop capability of C++ and Go socket code
anki_build_go_executable(ipctest_go "ipctest/go" "go" ${ANKI_SRCLIST_DIR})
anki_build_cxx_executable(ipctest_cpp ${ANKI_SRCLIST_DIR})
target_link_libraries(ipctest_cpp PRIVATE cti_messaging)

# cloud process
anki_build_go_executable(cloud_process "process" "go" ${ANKI_SRCLIST_DIR})
