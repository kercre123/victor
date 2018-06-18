cmake_minimum_required(VERSION 3.6)
project(cloud)

include(anki_build_cxx)
include(anki_build_go)
include(anki_build_util)

set(GATEWAY_GRPC_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/go/src/proto/external_interface")
set(PROTOBUF_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/proto/external_interface")
set(GATEWAY_GRPC_OUTPUT "${GATEWAY_GRPC_OUTPUT_DIR}/external_interface.pb.go" "${GATEWAY_GRPC_OUTPUT_DIR}/external_interface.pb.gw.go" "${GATEWAY_GRPC_OUTPUT_DIR}/messages.pb.go" "${GATEWAY_GRPC_OUTPUT_DIR}/messages.pb.gw.go")
set(PROTOBUF_OUTPUT "${PROTOBUF_OUTPUT_DIR}/messages.pb.h" "${PROTOBUF_OUTPUT_DIR}/messages.pb.cc")
set(PROTOBUF_OUTPUT ${PROTOBUF_OUTPUT} PARENT_SCOPE)
set(__extra_go_deps "${CLAD_GO_GEN_SRCS}" "${GATEWAY_GRPC_OUTPUT}" "${CLAD_GATEWAY_GEN_SRCS}" "${PROTOBUF_OUTPUT}")

if (NOT EXISTS "${ANKI_EXTERNAL_DIR}/chipper_key")
  message(WARNING "Need chipper_key file to be able to connect to cloud services!")
  message(WARNING "Check your VPN settings. You must be on VPN or office network to get chipper key.")
  message(FATAL_ERROR "Can't build cloud without chipper key")
endif()

set(chipper_flags "")
file(READ "${ANKI_EXTERNAL_DIR}/chipper_key" chipper_flags)
set(chipper_flags "-X \\\\\'anki/cloudproc.ChipperSecret=${chipper_flags}\\\\\'")

if(VICOS)
  set(__opus_build_dir ${CORETECH_EXTERNAL_DIR}/build/opus-vicos)
  add_library(opus SHARED IMPORTED)
  set_target_properties(opus PROPERTIES IMPORTED_LOCATION ${__opus_build_dir}/lib/libopus.so.0.6.1)
elseif(MACOSX)
  set(__opus_build_dir ${CORETECH_EXTERNAL_DIR}/build/opus-mac)
  add_library(opus STATIC IMPORTED)
  set_target_properties(opus PROPERTIES IMPORTED_LOCATION ${__opus_build_dir}/lib/libopus.a)
endif()

macro(_add_chipper_flags target_name)
  anki_go_set_ldflags(${target_name} ${chipper_flags})
  anki_go_add_include_dir(${target_name} ${__opus_build_dir}/include/opus)
  anki_go_add_c_library(${target_name} opus)
endmacro()

macro(_build_cloud_executable target_name)
  anki_build_go_executable(${target_name} ${ANKI_SRCLIST_DIR} "${__extra_go_deps}")
  anki_build_target_license(${target_name} "ANKI")
endmacro()

macro(_build_cloud_c_library target_name header_name)
  anki_build_go_c_library(${target_name} ${header_name} ${ANKI_SRCLIST_DIR} "${__extra_go_deps}")
  anki_build_target_license(${target_name} "ANKI")
endmacro()

# voicetest project (mac only)
if (MACOSX)

  # create Go target
  _build_cloud_c_library(voicego VOICEGO_HEADER)
  _add_chipper_flags(voicego)
  get_filename_component(VOICEGO_HEADER_DIR ${VOICEGO_HEADER} DIRECTORY)

  # create C target
  anki_build_cxx_executable(voicetest ${ANKI_SRCLIST_DIR} ${VOICEGO_HEADER})
  anki_build_target_license(voicetest "ANKI")
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
    opus
    ${SECURITY_FRAMEWORK}
  )


  # proctest - test victor audio process w/ laptop mic
  _build_cloud_c_library(proctest_go PROCTEST_GO_HEADER)
  _add_chipper_flags(proctest_go)
  get_filename_component(PROCTEST_HEADER_DIR ${PROCTEST_GO_HEADER} DIRECTORY)
  anki_build_cxx_executable(proctest ${ANKI_SRCLIST_DIR} ${PROCTEST_GO_HEADER})
  anki_build_target_license(proctest "ANKI")
  target_include_directories(proctest PUBLIC ${PROCTEST_HEADER_DIR})
  target_compile_definitions(proctest PUBLIC "HEADER_NAME=\"proctest_go.h\"")
  target_link_libraries(proctest PRIVATE util_audio proctest_go ${SECURITY_FRAMEWORK})
  anki_disable_default_build(proctest)

  _build_cloud_executable(opusupload)
  _add_chipper_flags(opusupload)

endif()

# ipctest - for testing interop capability of C++ and Go socket code
_build_cloud_executable(ipctest_go)
anki_disable_default_build(ipctest_go)
anki_build_cxx_executable(ipctest_cpp ${ANKI_SRCLIST_DIR})
anki_build_target_license(ipctest_cpp "ANKI")
target_link_libraries(ipctest_cpp PRIVATE cti_messaging)
anki_disable_default_build(ipctest_cpp)

# ipc server test things
_build_cloud_executable(servbench)
anki_disable_default_build(servbench)

# cloud process
_build_cloud_executable(vic-cloud)
_add_chipper_flags(vic-cloud)
# copy opusfile to lib dir
if (VICOS)
  configure_file(${__opus_build_dir}/lib/libopus.so.0.6.1 ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libopus.so.0 COPYONLY)
endif()

#
# wavtest
#
_build_cloud_executable(wavtest)
_add_chipper_flags(wavtest)

if (VICOS)
  anki_disable_default_build(wavtest)
endif()

#
# opusenc
#
_build_cloud_executable(opusenc)
_add_chipper_flags(opusenc)
anki_disable_default_build(opusenc)

#
# vic-gateway
#
# SDK translation service that interfaces between the engine and the external world.
#
set(GOPATH_GATEWAY "${CMAKE_CURRENT_SOURCE_DIR}/go")
set(GATEWAY_PROTOC_GENERATORS "${PROTOBUF_HOME}/go/bin/protoc-gen-go" "${PROTOBUF_HOME}/go/bin/protoc-gen-grpc-gateway")
set(GATEWAY_PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/gateway/external_interface.proto" "${CMAKE_CURRENT_SOURCE_DIR}/gateway/messages.proto")
set(PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/gateway/messages.proto")

add_custom_command(
  OUTPUT ${GATEWAY_GRPC_OUTPUT}
  COMMAND mkdir -p ${GATEWAY_GRPC_OUTPUT_DIR}
  COMMAND ${CMAKE_COMMAND} -E env "PATH=${PROTOBUF_HOME}/go/bin:$ENV{PATH}"
    ${PROTOBUF_HOME}/bin/protoc "-I${CMAKE_CURRENT_SOURCE_DIR}/gateway"
    -I"${PROTOBUF_HOME}/include"
    "-I${GOPATH_GATEWAY}/src"
    "-I${GOPATH_GATEWAY}/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis"
    "--go_out=plugins=grpc:${CMAKE_SOURCE_DIR}/generated/go/src/proto/external_interface"
    ${GATEWAY_PROTO_FILES}
  COMMAND ${CMAKE_COMMAND} -E env "PATH=${PROTOBUF_HOME}/go/bin:$ENV{PATH}"
    ${PROTOBUF_HOME}/bin/protoc -I"${CMAKE_CURRENT_SOURCE_DIR}/gateway"
    -I"${PROTOBUF_HOME}/include"
    -I"${GOPATH_GATEWAY}/src"
    -I"${GOPATH_GATEWAY}/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis"
    "--grpc-gateway_out=logtostderr=true:${CMAKE_SOURCE_DIR}/generated/go/src/proto/external_interface"
    ${GATEWAY_PROTO_FILES}
  DEPENDS "${GATEWAY_PROTO_FILES}" "${GATEWAY_PROTOC_GENERATORS}"
)
add_custom_command(
  OUTPUT ${PROTOBUF_OUTPUT}
  COMMAND mkdir -p ${PROTOBUF_OUTPUT_DIR}
  COMMAND ${PROTOBUF_HOME}/bin/protoc -I"${CMAKE_CURRENT_SOURCE_DIR}/gateway"
    -I"${PROTOBUF_HOME}/include"
    -I"${GOPATH_GATEWAY}/src"
    -I"${GOPATH_GATEWAY}/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis"
    "--cpp_out=${CMAKE_SOURCE_DIR}/generated/proto/external_interface"
    ${PROTO_FILES}
  DEPENDS "${PROTO_FILES}"
)
_build_cloud_executable(vic-gateway)
_add_chipper_flags(vic-gateway)
