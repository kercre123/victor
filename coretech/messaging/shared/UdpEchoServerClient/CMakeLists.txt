# products-cozmo/src/shared/comms/TcpSocket/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

project(UDPEchoServerClient)

set(   ../../include/anki/messaging/shared)

include_directories(
  ../../include
  ${  }
)


set(SERVER_SOURCE_FILES
  ../src/UdpServer.cpp
  UdpEchoServer.cpp
)

set(SERVER_HEADER_FILES
  ${  }/UdpServer.h
)

add_executable(UdpEchoServer ${SERVER_SOURCE_FILES} ${SERVER_HEADER_FILES})



set(CLIENT_SOURCE_FILES
  ../src/UdpClient.cpp
  UdpEchoClient.cpp
)

set(CLIENT_HEADER_FILES
  ${  }/UdpClient.h
)

add_executable(UdpEchoClient ${CLIENT_SOURCE_FILES} ${CLIENT_HEADER_FILES})

