cmake_minimum_required(VERSION 3.6)

project(engine_tools)

include(protobuf)
include(nng_ext)

include(anki_build_cxx)
include(anki_build_strip)

set(PROTOBUF_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/proto/arf")
set(PROTOBUF_CPP_PLUGIN "protocCppPlugin")
set(PROTOBUF_CPP_PLUGIN_PATH "${CMAKE_SOURCE_DIR}/tools/protobuf/plugin/${PROTOBUF_CPP_PLUGIN}")

FUNCTION(PREPEND var prefix)
   SET(listVar "")
   FOREACH(f ${ARGN})
      LIST(APPEND listVar "${prefix}/${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

file(STRINGS "${ANKI_SRCLIST_DIR}/arf_proto.srcs.lst" ARF_PROTO_SRCS)
string(REPLACE "proto/" "" ARF_PROTO_SRCS_NO_DIR "${ARF_PROTO_SRCS}")
string(REPLACE ".proto" ".pb.h" ARF_PROTO_CPP_GEN_HEADERS "${ARF_PROTO_SRCS_NO_DIR}")
string(REPLACE ".proto" ".pb.cc" ARF_PROTO_CPP_GEN_SRCS "${ARF_PROTO_SRCS_NO_DIR}")
PREPEND(ARF_PROTO_CPP_GEN_HEADERS "${PROTOBUF_OUTPUT_DIR}/" "${ARF_PROTO_CPP_GEN_HEADERS}")
PREPEND(ARF_PROTO_CPP_GEN_SRCS "${PROTOBUF_OUTPUT_DIR}/" "${ARF_PROTO_CPP_GEN_SRCS}")

set(ARF_PROTO_HOME_DIR "${CMAKE_SOURCE_DIR}/platform/arf/proto")

add_custom_command(
  OUTPUT ${ARF_PROTO_CPP_GEN_HEADERS} ${ARF_PROTO_CPP_GEN_SRCS}
  COMMAND mkdir -p ${PROTOBUF_OUTPUT_DIR}
  COMMAND ${PROTOBUF_HOME}/bin/protoc -I"${ARF_PROTO_HOME_DIR}"
    -I"${PROTOBUF_HOME}/include"
    "--plugin=protoc-gen-${PROTOBUF_CPP_PLUGIN}=${PROTOBUF_CPP_PLUGIN_PATH}"
    "--cpp_out=${PROTOBUF_OUTPUT_DIR}"
    "--${PROTOBUF_CPP_PLUGIN}_out=${PROTOBUF_OUTPUT_DIR}"
    ${ARF_PROTO_SRCS_NO_DIR}
  DEPENDS "${ARF_PROTO_SRCS}" "${PROTOBUF_CPP_PLUGIN_PATH}"
)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_FLAGS "")

if (VICOS)
  set(PLATFORM_LIBS log)
elseif (MACOSX)
  find_library(FOUNDATION Foundation)
  set(PLATFORM_LIBS
    ${FOUNDATION}
  )
endif()

anki_build_cxx_library(arf ${ANKI_SRCLIST_DIR} ${ARF_PROTO_CPP_GEN_SRCS} ${ARF_PROTO_CPP_GEN_HEADERS})
anki_build_target_license(arf "ANKI")

target_link_libraries(arf
  PUBLIC
  util
  # external
  ${PROTOBUF_LIBS}
  # platform
  ${NNG_LIBS}
  pthread # Hack to get pthread to appear on linker line after nng
  ${PLATFORM_LIBS}
  ${ASAN_EXE_LINKER_FLAGS}
)

anki_build_strip(TARGET arf)

target_include_directories(arf
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PROTOBUF_OUTPUT_DIR}>
  $<BUILD_INTERFACE:${NNG_INCLUDE_PATH}>
)

anki_build_cxx_executable(subscriber ${ANKI_SRCLIST_DIR})
anki_build_target_license(subscriber "ANKI")

target_link_libraries(subscriber
  PRIVATE
  arf
  # platform
  ${PLATFORM_LIBS}
  ${ASAN_EXE_LINKER_FLAGS}
)

anki_build_cxx_executable(publisher ${ANKI_SRCLIST_DIR})
anki_build_target_license(publisher "ANKI")

target_link_libraries(publisher
  PRIVATE
  arf
  # platform
  ${PLATFORM_LIBS}
  ${ASAN_EXE_LINKER_FLAGS}
)