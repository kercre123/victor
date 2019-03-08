cmake_minimum_required(VERSION 3.6)

project(arf_tester)

include(anki_build_cxx)
include(anki_build_strip)
include(protobuf)
include(nng_ext)

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

set(ARF_PROTO_HOME_DIR "${CMAKE_SOURCE_DIR}/platform/arf_tester/proto")

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

anki_build_cxx_executable(arf-tester ${ANKI_SRCLIST_DIR} ${ARF_PROTO_CPP_GEN_SRCS} ${ARF_PROTO_CPP_GEN_HEADERS})
anki_build_target_license(arf-tester "ANKI")

target_link_libraries(arf-tester
  PRIVATE
  # anki libs
  arf
  util
  cti_common
  cti_vision
  engine_clad # imageTypes.h
  robot_interface # cozmoConfig.h
  cameraService
  # external
  ${PROTOBUF_LIBS}
  # platform
  ${PLATFORM_LIBS}
  ${ASAN_EXE_LINKER_FLAGS}
)

target_include_directories(arf-tester
  PUBLIC  
  $<BUILD_INTERFACE:${PROTOBUF_OUTPUT_DIR}>
)

target_compile_definitions(arf-tester
  PRIVATE
  ANKICORETECH_USE_OPENCV=1
  ${PLATFORM_COMPILE_DEFS}
)

anki_build_strip(TARGET arf-tester)