cmake_minimum_required(VERSION 3.6)
project(cloud)

include(anki_build_cxx)
include(anki_build_go)
include(anki_build_util)
include(breakpad)

# Grab the gateway files
file(STRINGS "${ANKI_SRCLIST_DIR}/gateway_grpc.srcs.lst" GATEWAY_GRPC_SRCS)
file(STRINGS "${ANKI_SRCLIST_DIR}/gateway_proto.srcs.lst" GATEWAY_PROTO_SRCS)

# exclude grpc file from proto sources
list(FILTER GATEWAY_PROTO_SRCS EXCLUDE REGEX ".*external_interface.proto")

set(GATEWAY_GRPC_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/go/src/proto/external_interface")
set(PROTOBUF_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/proto/external_interface")

# Formulate the generated file names
string(REPLACE ".proto" ".pb.go" GATEWAY_PROTO_GO_GEN_SRCS "${GATEWAY_PROTO_SRCS};${GATEWAY_GRPC_SRCS}")
string(REPLACE ".proto" ".pb.gw.go" GATEWAY_GRPC_GO_GEN_SRCS "${GATEWAY_GRPC_SRCS}")
string(REPLACE ".proto" ".pb.h" GATEWAY_PROTO_CPP_GEN_HEADERS "${GATEWAY_PROTO_SRCS}")
string(REPLACE ".proto" ".pb.cc" GATEWAY_PROTO_CPP_GEN_SRCS "${GATEWAY_PROTO_SRCS}")

# and give those generated files the appropriate pathing
string(REPLACE "gateway/" "${GATEWAY_GRPC_OUTPUT_DIR}/" GATEWAY_PROTO_GO_GEN_SRCS "${GATEWAY_PROTO_GO_GEN_SRCS}")
string(REPLACE "gateway/" "${GATEWAY_GRPC_OUTPUT_DIR}/" GATEWAY_GRPC_GO_GEN_SRCS "${GATEWAY_GRPC_GO_GEN_SRCS}")
string(REPLACE "gateway/" "${PROTOBUF_OUTPUT_DIR}/" GATEWAY_PROTO_CPP_GEN_HEADERS "${GATEWAY_PROTO_CPP_GEN_HEADERS}")
string(REPLACE "gateway/" "${PROTOBUF_OUTPUT_DIR}/" GATEWAY_PROTO_CPP_GEN_SRCS "${GATEWAY_PROTO_CPP_GEN_SRCS}")

# Full paths for the proto files
string(REPLACE "gateway/" "${CMAKE_CURRENT_SOURCE_DIR}/gateway/" GATEWAY_GRPC_SRCS "${GATEWAY_GRPC_SRCS}")
string(REPLACE "gateway/" "${CMAKE_CURRENT_SOURCE_DIR}/gateway/" GATEWAY_PROTO_SRCS "${GATEWAY_PROTO_SRCS}")

# Expose the necessary files for the engine to build thease into it
set(GATEWAY_PROTO_CPP_GEN_HEADERS "${GATEWAY_PROTO_CPP_GEN_HEADERS}" PARENT_SCOPE)
set(GATEWAY_PROTO_CPP_GEN_SRCS "${GATEWAY_PROTO_CPP_GEN_SRCS}" PARENT_SCOPE)
set(GATEWAY_PROTO_SRCS "${GATEWAY_PROTO_SRCS}" PARENT_SCOPE)
set(PROTOBUF_OUTPUT_DIR "${PROTOBUF_OUTPUT_DIR}" PARENT_SCOPE)

set(__extra_go_deps "${CLAD_GO_GEN_SRCS}" "${GATEWAY_PROTO_GO_GEN_SRCS}" "${GATEWAY_GRPC_GO_GEN_SRCS}" "${CLAD_GATEWAY_GEN_SRCS}")

if (NOT EXISTS "${ANKI_EXTERNAL_DIR}/chipper_key")
  message(WARNING "Need chipper_key file to be able to connect to cloud services!")
  message(WARNING "Check your VPN settings. You must be on VPN or office network to get chipper key.")
  message(FATAL_ERROR "Can't build cloud without chipper key")
endif()

if (NOT EXISTS "${ANKI_EXTERNAL_DIR}/aws_credentials")
  message(WARNING "Need aws_keys file to be able to connect to upload logs!")
  message(WARNING "Check your VPN settings. You must be on VPN or office network to get AWS credentials.")
  message(FATAL_ERROR "Can't build cloud without AWS credentials")
endif()

set(chipper_flags "")
file(READ "${ANKI_EXTERNAL_DIR}/chipper_key" chipper_flags)
file(READ "${ANKI_EXTERNAL_DIR}/aws_credentials" logcollector_flags)
set(chipper_flags "-X \\\\\'anki/voice.ChipperSecret=${chipper_flags}\\\\\' -X \\\\\'anki/logcollector.AwsCredentials=${logcollector_flags}\\\\\'")

if(VICOS)
  set(__opus_build_dir ${CORETECH_EXTERNAL_DIR}/build/opus-vicos)
  add_library(opus SHARED IMPORTED)
  anki_build_target_license(opus "BSD-3,${CMAKE_SOURCE_DIR}/licenses/opus.license")
  set_target_properties(opus PROPERTIES IMPORTED_LOCATION ${__opus_build_dir}/lib/libopus.so.0.6.1)
elseif(MACOSX)
  set(__opus_build_dir ${CORETECH_EXTERNAL_DIR}/build/opus-mac)
  add_library(opus STATIC IMPORTED)
  anki_build_target_license(opus "BSD-3,${CMAKE_SOURCE_DIR}/licenses/opus.license")
  set_target_properties(opus PROPERTIES IMPORTED_LOCATION ${__opus_build_dir}/lib/libopus.a)
endif()


macro(_add_chipper_flags target_name)
  anki_go_set_ldflags(${target_name} ${chipper_flags})
  anki_go_add_include_dir(${target_name} ${__opus_build_dir}/include/opus)
  anki_go_add_c_library(${target_name} opus)
endmacro()

macro(_add_crash_reporter_flags target_name)
  if (VICOS)
    anki_go_set_cgo_cppflags(${target_name} -DANKI_PLATFORM_VICOS)
    anki_go_add_include_dir(${target_name} ${CMAKE_SOURCE_DIR})
    anki_go_add_c_library(${target_name} victorCrashReports)
    anki_go_add_c_library(${target_name} breakpad_client)
    anki_go_add_c_library(${target_name} ankiutil)
    anki_go_add_c_library(${target_name} log)
    #
    # Our build macros don't handle the case where a library (util) has a custom output name (ankiutil).
    # Add an explicit dependency to ensure that the library is built before our target.
    add_dependencies(${target_name} util)
  endif()
endmacro()

macro(_build_cloud_executable target_name)
  anki_build_go_executable(${target_name} ${ANKI_SRCLIST_DIR} "${__extra_go_deps}")
  get_target_property(__util_include_dirs util INCLUDE_DIRECTORIES)
  anki_go_add_include_dir(${target_name} ${__util_include_dirs})
endmacro()

macro(_build_cloud_c_library target_name header_name)
  anki_build_go_c_library(${target_name} ${header_name} ${ANKI_SRCLIST_DIR} "${__extra_go_deps}")
endmacro()

# voicetest project (mac only)
if (MACOSX)

  # create Go target
  _build_cloud_c_library(voicego VOICEGO_HEADER)
  anki_build_target_license(voicego "ANKI")
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
  anki_build_target_license(proctest_go "ANKI")
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

  _build_cloud_executable(sessiondl)
  anki_disable_default_build(sessiondl)

  _build_cloud_executable(tokprovision)
  anki_build_target_license(tokprovision "ANKI")

endif()

# CLI tool for testing logcollector (log uploads)
_build_cloud_executable(logcollectorcli)
anki_disable_default_build(logcollectorcli)

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

# see also VIC-6541 Solve keeping list of approved open-source licenses up-to-date with the license checks

anki_build_target_license(vic-cloud
  "ANKI"
  "Apache-2.0,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/aws/aws-sdk-go/vendor/github.com/go-ini/ini/LICENSE"
  "Apache-2.0,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/aws/aws-sdk-go/vendor/github.com/jmespath/go-jmespath/LICENSE"
  "Apache-2.0,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/golang/glog/LICENSE"
  "Apache-2.0,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/google/martian/LICENSE"
  "Apache-2.0,${CMAKE_SOURCE_DIR}/cloud/go/src/google.golang.org/genproto/LICENSE"
  "Apache-2.0,${CMAKE_SOURCE_DIR}/cloud/go/src/google.golang.org/grpc/LICENSE"
  "BSD-2,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/aalpern/go-metrics/LICENSE"
  "BSD-3,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/google/uuid/LICENSE"
  "BSD-3,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/gorilla/context/LICENSE"
  "BSD-3,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/grd/ogg/LICENSE"
  "BSD-3,${CMAKE_SOURCE_DIR}/cloud/go/src/golang.org/x/text/LICENSE"
  "MIT,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/anki/opus-go/libopus/LICENSE"
  "MIT,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/cenkalti/backoff/LICENSE"
  "MIT,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/dgrijalva/jwt-go/LICENSE"
  "MIT,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/gwatts/iniconf/LICENSE"
  "MIT,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/gwatts/rootcerts/LICENSE"
  "MIT,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/jawher/mow.cli/LICENSE"
  "MIT,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/satori/go.uuid/LICENSE"
)

_add_chipper_flags(vic-cloud)
_add_crash_reporter_flags(vic-cloud)

# copy opusfile to lib dir
if (VICOS)
  configure_file(${__opus_build_dir}/lib/libopus.so.0.6.1 ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libopus.so.0 COPYONLY)
endif()

#
# wavtest
#
_build_cloud_executable(wavtest)
anki_build_target_license(wavtest "ANKI")
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

# Generate grpc go files
add_custom_command(
  OUTPUT ${GATEWAY_PROTO_GO_GEN_SRCS} ${GATEWAY_GRPC_GO_GEN_SRCS}
  COMMAND mkdir -p ${GATEWAY_GRPC_OUTPUT_DIR}
  COMMAND ${CMAKE_COMMAND} -E env "PATH=${PROTOBUF_HOME}/go/bin:$ENV{PATH}"
    ${PROTOBUF_HOME}/bin/protoc "-I${CMAKE_CURRENT_SOURCE_DIR}/gateway"
    -I"${PROTOBUF_HOME}/include"
    "-I${GOPATH_GATEWAY}/src"
    "-I${GOPATH_GATEWAY}/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis"
    "--go_out=plugins=grpc:${GATEWAY_GRPC_OUTPUT_DIR}"
    ${GATEWAY_GRPC_SRCS} ${GATEWAY_PROTO_SRCS}
  COMMAND ${CMAKE_COMMAND} -E env "PATH=${PROTOBUF_HOME}/go/bin:$ENV{PATH}"
    ${PROTOBUF_HOME}/bin/protoc -I"${CMAKE_CURRENT_SOURCE_DIR}/gateway"
    -I"${PROTOBUF_HOME}/include"
    -I"${GOPATH_GATEWAY}/src"
    -I"${GOPATH_GATEWAY}/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis"
    "--grpc-gateway_out=logtostderr=true:${GATEWAY_GRPC_OUTPUT_DIR}"
    ${GATEWAY_GRPC_SRCS} ${GATEWAY_PROTO_SRCS}
  DEPENDS ${GATEWAY_GRPC_SRCS} ${GATEWAY_PROTO_SRCS}
)

_build_cloud_executable(vic-gateway)

# see also VIC-6541 Solve keeping list of approved open-source licenses up-to-date with the license checks

anki_build_target_license(vic-gateway
  "ANKI"
  "Apache-2.0,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/aws/aws-sdk-go/vendor/github.com/go-ini/ini/LICENSE"
  "Apache-2.0,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/aws/aws-sdk-go/vendor/github.com/jmespath/go-jmespath/LICENSE"
  "Apache-2.0,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/golang/glog/LICENSE"
  "Apache-2.0,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/google/martian/LICENSE"
  "Apache-2.0,${CMAKE_SOURCE_DIR}/cloud/go/src/google.golang.org/genproto/LICENSE"
  "Apache-2.0,${CMAKE_SOURCE_DIR}/cloud/go/src/google.golang.org/grpc/LICENSE"
  "BSD-2,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/aalpern/go-metrics/LICENSE"
  "BSD-3,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/google/uuid/LICENSE"
  "BSD-3,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/gorilla/context/LICENSE"
  "BSD-3,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/grd/ogg/LICENSE"
  "BSD-3,${CMAKE_SOURCE_DIR}/cloud/go/src/golang.org/x/text/LICENSE"
  "MIT,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/anki/opus-go/libopus/LICENSE"
  "MIT,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/cenkalti/backoff/LICENSE"
  "MIT,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/dgrijalva/jwt-go/LICENSE"
  "MIT,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/gwatts/iniconf/LICENSE"
  "MIT,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/gwatts/rootcerts/LICENSE"
  "MIT,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/jawher/mow.cli/LICENSE"
  "MIT,${CMAKE_SOURCE_DIR}/cloud/go/src/github.com/satori/go.uuid/LICENSE"
)

_add_chipper_flags(vic-gateway)
_add_crash_reporter_flags(vic-gateway)

# Install certs for mac build of vic-gateway
if (MACOSX)
  set(GATEWAY_MAC_CERTS
    "/tmp/anki/gateway/trust.cert"
    "/tmp/anki/gateway/trust.key"
  )

  add_custom_target(
    GenerateGatewayMacCerts ALL
    DEPENDS ${GATEWAY_MAC_CERTS}
  )

  add_custom_command(
    OUTPUT ${GATEWAY_MAC_CERTS}
    COMMAND make create_certs_mac
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tools/sdk/grpc_tools"
    COMMENT "Creating mac certs for vic-gateway"
  )
endif()
