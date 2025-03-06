#
# cloud/CMakeLists.txt
#
cmake_minimum_required(VERSION 3.6)

project(cloud)

include(anki_build_cxx)
include(anki_build_util)
include(breakpad)
include(webots)

#
# Gateway Protbuf and GRPC configuration
#

# Grab the gateway files
file(STRINGS "${ANKI_SRCLIST_DIR}/gateway_grpc.srcs.lst" GATEWAY_GRPC_SRCS)
file(STRINGS "${ANKI_SRCLIST_DIR}/gateway_proto.srcs.lst" GATEWAY_PROTO_SRCS)

# exclude grpc file from proto sources
list(FILTER GATEWAY_PROTO_SRCS EXCLUDE REGEX ".*external_interface.proto")

set(GATEWAY_GRPC_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/go/src/proto/external_interface")
set(PROTOBUF_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/proto/external_interface")
set(GATEWAY_PROTO_HOME_DIR "${CMAKE_SOURCE_DIR}/tools/protobuf/gateway")
set(GATEWAY_PROTO_HOME_DIR_RELATIVE "../tools/protobuf/gateway")

# Formulate the generated file names
string(REPLACE ".proto" ".pb.go" GATEWAY_PROTO_GO_GEN_SRCS "${GATEWAY_PROTO_SRCS};${GATEWAY_GRPC_SRCS}")
string(REPLACE ".proto" ".pb.gw.go" GATEWAY_GRPC_GO_GEN_SRCS "${GATEWAY_GRPC_SRCS}")
string(REPLACE ".proto" ".pb.h" GATEWAY_PROTO_CPP_GEN_HEADERS "${GATEWAY_PROTO_SRCS}")
string(REPLACE ".proto" ".pb.cc" GATEWAY_PROTO_CPP_GEN_SRCS "${GATEWAY_PROTO_SRCS}")

# and give those generated files the appropriate pathing
string(REPLACE "${GATEWAY_PROTO_HOME_DIR_RELATIVE}/public/" "${GATEWAY_GRPC_OUTPUT_DIR}/" GATEWAY_PROTO_GO_GEN_SRCS "${GATEWAY_PROTO_GO_GEN_SRCS}")
string(REPLACE "${GATEWAY_PROTO_HOME_DIR_RELATIVE}/public/" "${GATEWAY_GRPC_OUTPUT_DIR}/" GATEWAY_GRPC_GO_GEN_SRCS "${GATEWAY_GRPC_GO_GEN_SRCS}")
string(REPLACE "${GATEWAY_PROTO_HOME_DIR_RELATIVE}/public/" "${PROTOBUF_OUTPUT_DIR}/" GATEWAY_PROTO_CPP_GEN_HEADERS "${GATEWAY_PROTO_CPP_GEN_HEADERS}")
string(REPLACE "${GATEWAY_PROTO_HOME_DIR_RELATIVE}/public/" "${PROTOBUF_OUTPUT_DIR}/" GATEWAY_PROTO_CPP_GEN_SRCS "${GATEWAY_PROTO_CPP_GEN_SRCS}")

# Full paths for the proto files
string(REPLACE "${GATEWAY_PROTO_HOME_DIR_RELATIVE}" "${GATEWAY_PROTO_HOME_DIR}/" GATEWAY_GRPC_SRCS "${GATEWAY_GRPC_SRCS}")
string(REPLACE "${GATEWAY_PROTO_HOME_DIR_RELATIVE}" "${GATEWAY_PROTO_HOME_DIR}/" GATEWAY_PROTO_SRCS "${GATEWAY_PROTO_SRCS}")

# Expose the necessary files for the engine to build thease into it
set(GATEWAY_PROTO_CPP_GEN_HEADERS "${GATEWAY_PROTO_CPP_GEN_HEADERS}" PARENT_SCOPE)
set(GATEWAY_PROTO_CPP_GEN_SRCS "${GATEWAY_PROTO_CPP_GEN_SRCS}" PARENT_SCOPE)
set(GATEWAY_PROTO_SRCS "${GATEWAY_PROTO_SRCS}" PARENT_SCOPE)
set(PROTOBUF_OUTPUT_DIR "${PROTOBUF_OUTPUT_DIR}" PARENT_SCOPE)
set(GATEWAY_PROTO_HOME_DIR "${GATEWAY_PROTO_HOME_DIR}" PARENT_SCOPE)
