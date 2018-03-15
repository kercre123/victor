cmake_minimum_required(VERSION 3.6)
project(cloud)

include(anki_build_cxx)
include(anki_build_go)
include(anki_build_util)

if (NOT EXISTS "${ANKI_EXTERNAL_DIR}/chipper_key")
  message(WARNING "Need chipper_key file to be able to connect to cloud services!")
  message(WARNING "Check your VPN settings. You must be on VPN or office network to get chipper key.")
  message(FATAL_ERROR "Can't build cloud without chipper key")
endif()

set(chipper_flags "")
file(READ "${ANKI_EXTERNAL_DIR}/chipper_key" chipper_flags)
set(chipper_flags "-X \\\\\'anki/cloudproc.ChipperSecret=${chipper_flags}\\\\\'")

if (ANDROID)
  set(__opus_build_dir ${CORETECH_EXTERNAL_DIR}/build/opus-linux)
  add_library(opus SHARED IMPORTED)
  set_target_properties(opus PROPERTIES IMPORTED_LOCATION ${__opus_build_dir}/lib/libopus.so)
else()
  set(__opus_build_dir ${CORETECH_EXTERNAL_DIR}/build/opus-mac)
  add_library(opus STATIC IMPORTED)
  set_target_properties(opus PROPERTIES IMPORTED_LOCATION ${__opus_build_dir}/lib/libopus.a)
endif()

macro(_add_chipper_flags target_name)
  anki_go_set_ldflags(${target_name} ${chipper_flags})
  anki_go_add_include_dir(${target_name} ${__opus_build_dir}/include/opus)
  anki_go_add_c_library(${target_name} opus)
endmacro()

# voicetest project (mac only)
if (MACOSX)

  # create Go target
  anki_build_go_c_library(voicego VOICEGO_HEADER ${ANKI_SRCLIST_DIR})
  _add_chipper_flags(voicego)
  get_filename_component(VOICEGO_HEADER_DIR ${VOICEGO_HEADER} DIRECTORY)

  # create C target
  anki_build_cxx_executable(voicetest ${ANKI_SRCLIST_DIR} ${VOICEGO_HEADER})
  anki_disable_default_build(voicetest)

  # tell voicetest where to find voicego.h
  target_include_directories(voicetest
    PUBLIC
    ${VOICEGO_HEADER_DIR}
  )

  # need security framework too (SAI dependencies??)
  find_library(SECURITY_FRAMEWORK Security)
  if(NOT SECURITY_FRAMEWORK)
    message(FATAL_ERROR "Security not found")
  endif()

  # link audio and go libraries
  target_link_libraries(voicetest
    PRIVATE
    util_audio
    voicego
    ${SECURITY_FRAMEWORK}
  )


  # proctest - test victor audio process w/ laptop mic
  anki_build_go_c_library(proctest_go PROCTEST_GO_HEADER ${ANKI_SRCLIST_DIR})
  _add_chipper_flags(proctest_go)
  get_filename_component(PROCTEST_HEADER_DIR ${PROCTEST_GO_HEADER} DIRECTORY)
  anki_build_cxx_executable(proctest ${ANKI_SRCLIST_DIR} ${PROCTEST_GO_HEADER})
  target_include_directories(proctest PUBLIC ${PROCTEST_HEADER_DIR})
  target_compile_definitions(proctest PUBLIC "HEADER_NAME=\"proctest_go.h\"")
  target_link_libraries(proctest PRIVATE util_audio proctest_go ${SECURITY_FRAMEWORK})
  anki_disable_default_build(proctest)

  anki_build_go_executable(opusupload ${ANKI_SRCLIST_DIR})
  _add_chipper_flags(opusupload)

endif()

# ipctest - for testing interop capability of C++ and Go socket code
anki_build_go_executable(ipctest_go ${ANKI_SRCLIST_DIR})
anki_disable_default_build(ipctest_go)
anki_build_cxx_executable(ipctest_cpp ${ANKI_SRCLIST_DIR})
target_link_libraries(ipctest_cpp PRIVATE cti_messaging)
anki_disable_default_build(ipctest_cpp)

# ipc server test things
anki_build_go_executable(servbench ${ANKI_SRCLIST_DIR})
anki_disable_default_build(servbench)

# cloud process
anki_build_go_executable(vic-cloud ${ANKI_SRCLIST_DIR})
_add_chipper_flags(vic-cloud)
# copy opusfile to lib dir
if (ANDROID)
  configure_file(${__opus_build_dir}/lib/libopus.so ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} COPYONLY)
endif()

#
# wavtest
#
anki_build_go_executable(wavtest ${ANKI_SRCLIST_DIR})
_add_chipper_flags(wavtest)

if (ANDROID)
  anki_disable_default_build(wavtest)
endif()

#
# opusenc
#
anki_build_go_executable(opusenc ${ANKI_SRCLIST_DIR})
_add_chipper_flags(opusenc)
anki_disable_default_build(opusenc)
