# products-cozmo/src/shared/comms/TcpSocket/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

project(EchoServerClient)

set(INCLUDE_DIR ../../include/anki/messaging/shared)

include_directories(
  ../../include
  ${INCLUDE_DIR}
)


set(SERVER_SOURCE_FILES
  ../src/TcpMultiClientServer.cpp
  TcpMultiClientServerMain.cpp
)

set(SERVER_HEADER_FILES
  ${INCLUDE_DIR}/TcpServer.h
)

set(CMAKE_CXX_FLAGS "-std=c++11")

add_executable(MultiClientEchoServer ${SERVER_SOURCE_FILES} ${SERVER_HEADER_FILES})



set(CLIENT_SOURCE_FILES
  ../src/TcpClient.cpp
  TcpClientMain.cpp
)

set(CLIENT_HEADER_FILES
  ${INCLUDE_DIR}/TcpClient.h
)

add_executable(EchoClient ${CLIENT_SOURCE_FILES} ${CLIENT_HEADER_FILES})

