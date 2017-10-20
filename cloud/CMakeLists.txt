cmake_minimum_required(VERSION 3.6)
project(cloud)

include(anki_build_cxx)

# voicetest project (mac only)
if (MACOSX)

  # find go sources and assign to VOICETEST_GO_SRC
  execute_process(
    COMMAND find "${CMAKE_CURRENT_SOURCE_DIR}/voicetest/go" -iname *.go
    OUTPUT_VARIABLE VOICETEST_GO_SRC
  )

  # build C library from go code
  set(VOICEGO_OUT "${CMAKE_CURRENT_BINARY_DIR}/voicetest/go/voicego.a")
  # C code needs to include the generated voicego.h
  get_filename_component(VOICEGO_OUT_DIR ${VOICEGO_OUT} DIRECTORY)
  set(VOICEGO_HEADER "${VOICEGO_OUT_DIR}/voicego.h")

  add_custom_command(
    OUTPUT ${VOICEGO_OUT} ${VOICEGO_HEADER}
    COMMAND go build -buildmode=c-archive -o ${VOICEGO_OUT} ${VOICETEST_GO_SRC}
    DEPENDS ${VOICETEST_GO_SRC}
  )
  add_custom_target(voicego_target DEPENDS ${VOICEGO_OUT})

  # cmake mumbo jumbo I don't totally understand so that voicetest
  # gets the right dependencies for voicego.a
  add_library(voicego STATIC IMPORTED GLOBAL)
  add_dependencies(voicego voicego_target)
  set_property(TARGET voicego PROPERTY IMPORTED_LOCATION ${VOICEGO_OUT})

  # create C target
  anki_build_cxx_executable(voicetest ${ANKI_SRCLIST_DIR} ${VOICEGO_HEADER})

  # tell voicetest where to find voicego.h
  target_include_directories(voicetest
    PUBLIC
    ${VOICEGO_OUT_DIR}
  )

  # link audio and go libraries
  target_link_libraries(voicetest
    PRIVATE
    util_audio
    voicego
  )
endif()
