# products-cozmo/src/shared/comms/TcpSocket/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

project(EchoServerClient)

set(SERVER_SOURCE_FILES
	TcpServer.cpp
	TcpServerMain.cpp
)

set(SERVER_HEADER_FILES
	TcpServer.h
)

add_executable(EchoServer ${SERVER_SOURCE_FILES} ${SERVER_HEADER_FILES})



set(CLIENT_SOURCE_FILES
	TcpClient.cpp
	TcpClientMain.cpp
)

set(CLIENT_HEADER_FILES
	TcpClient.h
)

add_executable(EchoClient ${CLIENT_SOURCE_FILES} ${CLIENT_HEADER_FILES})

