cmake_minimum_required(VERSION 3.6)

if (VICOS)

project(vic-switchboard)

add_definitions(-DGPR_FORBID_UNREACHABLE_CODE=0)
set (CMAKE_BUILD_WITH_INSTALL_RPATH TRUE CACHE BOOL TRUE FORCE)
set(HAVE_STD_REGEX TRUE CACHE BOOL TRUE FORCE)
set(HAVE_POSIX_REGEX TRUE CACHE BOOL TRUE FORCE)
set(HAVE_STEADY_CLOCK TRUE CACHE BOOL TRUE FORCE)
add_subdirectory(anki-thirdparty/grpc ${CMAKE_CURRENT_BINARY_DIR}/grpc EXCLUDE_FROM_ALL)
message(STATUS "Using gRPC via add_subdirectory.")

# After using add_subdirectory, we can now use the grpc targets directly from
# this build.
set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_PROTOBUF_PROTOC protoc)
set(_GRPC_GRPCPP_UNSECURE grpc++_unsecure)
set(_GRPC_CPP_PLUGIN_EXECUTABLE /usr/local/bin/grpc_cpp_plugin)

set(GRPC_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/grpc")

# get_filename_component(hw_proto "protos/helloworld.proto" ABSOLUTE)
get_filename_component(proto_path "protos" ABSOLUTE)
get_filename_component(googleapi_proto_path "/Users/shawnb/public_repos/grpc-gateway/third_party/googleapis/" ABSOLUTE)


set(proto_srcs "${GRPC_OUTPUT}/sdk_interface.pb.cc" "${GRPC_OUTPUT}/google/api/annotations.pb.cc" "${GRPC_OUTPUT}/google/api/http.pb.cc")
set(proto_hdrs "${GRPC_OUTPUT}/sdk_interface.pb.h" "${GRPC_OUTPUT}/google/api/annotations.pb.h" "${GRPC_OUTPUT}/google/api/http.pb.h")
set(grpc_srcs  "${GRPC_OUTPUT}/sdk_interface.grpc.pb.cc" "${GRPC_OUTPUT}/google/api/annotations.grpc.pb.cc" "${GRPC_OUTPUT}/google/api/http.grpc.pb.cc")
set(grpc_hdrs  "${GRPC_OUTPUT}/sdk_interface.grpc.pb.h" "${GRPC_OUTPUT}/google/api/annotations.grpc.pb.h" "${GRPC_OUTPUT}/google/api/http.grpc.pb.h")
add_custom_command(
      OUTPUT "${GRPC_OUTPUT}/sdk_interface.pb.cc" "${GRPC_OUTPUT}/google/api/annotations.pb.cc" "${GRPC_OUTPUT}/google/api/http.pb.cc" 
      "${GRPC_OUTPUT}/sdk_interface.pb.h" "${GRPC_OUTPUT}/google/api/annotations.pb.h" "${GRPC_OUTPUT}/google/api/http.pb.h" 
      "${GRPC_OUTPUT}/sdk_interface.grpc.pb.cc" "${GRPC_OUTPUT}/google/api/annotations.grpc.pb.cc" "${GRPC_OUTPUT}/google/api/http.grpc.pb.cc" 
      "${GRPC_OUTPUT}/sdk_interface.grpc.pb.h" "${GRPC_OUTPUT}/google/api/annotations.grpc.pb.h" "${GRPC_OUTPUT}/google/api/http.grpc.pb.h"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${GRPC_OUTPUT}"
        --cpp_out "${GRPC_OUTPUT}"
        -I "${proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/protos/sdk_interface.proto"
        "${CMAKE_CURRENT_SOURCE_DIR}/protos/google/api/annotations.proto"
        "${CMAKE_CURRENT_SOURCE_DIR}/protos/google/api/http.proto"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/protos/sdk_interface.proto"
              "${CMAKE_CURRENT_SOURCE_DIR}/protos/google/api/annotations.proto"
              "${CMAKE_CURRENT_SOURCE_DIR}/protos/google/api/http.proto")

include(anki_build_cxx)
include(sodium)

anki_build_cxx_executable(vic-switchboard ${ANKI_SRCLIST_DIR} 
"${GRPC_OUTPUT}/sdk_interface.pb.cc" "${GRPC_OUTPUT}/google/api/annotations.pb.cc" "${GRPC_OUTPUT}/google/api/http.pb.cc" 
"${GRPC_OUTPUT}/sdk_interface.grpc.pb.cc" "${GRPC_OUTPUT}/google/api/annotations.grpc.pb.cc" "${GRPC_OUTPUT}/google/api/http.grpc.pb.cc")

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
  
set(PLATFORM_LIBS
  log
  c
)

target_link_libraries(vic-switchboard
PRIVATE
  ${PLATFORM_LIBS}
  sdk_clad_cpp
  robot_clad_cpplite
  cti_common
  cti_messaging
  engine_clad
  ${SODIUM_LIBS}
  jsoncpp
  libglib-2.0.so
  libgio-2.0.so
  libgobject-2.0.so
  libffi.so.6
  libdl.so.2
  libz.so.1
  libresolv.so.2
  libgmodule-2.0.so.0
  libpcre.so.1
  libcutils.so.0
  # civetweb
  ${_GRPC_GRPCPP_UNSECURE}
  ${_PROTOBUF_LIBPROTOBUF}
)

target_compile_definitions(vic-switchboard
  PRIVATE
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(vic-switchboard
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/switchboardd>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cutils>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/anki-ble>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/anki-wifi>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libev>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/signals>
  $<BUILD_INTERFACE:${GRPC_OUTPUT}>
  $<BUILD_INTERFACE:${CORETECH_EXTERNAL_DIR}/build/libsodium/vicos/include>
  $<BUILD_INTERFACE:${VICOS_SDK}/sysroot/usr/include/gio-unix-2.0>
  $<BUILD_INTERFACE:${VICOS_SDK}/sysroot/usr/include/glib-2.0>
  $<BUILD_INTERFACE:${VICOS_SDK}/sysroot/usr/lib/glib-2.0/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/lib/util/source/3rd>
  ${PLATFORM_INCLUDES}
)

anki_build_strip(TARGET vic-switchboard)

endif(VICOS)
