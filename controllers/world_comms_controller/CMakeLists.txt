# products-cozmo/simulator/controllers/world_comms_controller/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

project(world_comms_controller)

# Include paths
include_directories( ${COZMO_SRC_DIR}/robot/
                     ${COZMO_SRC_DIR}/shared/
                     ${WEBOTS_CPP_INCLUDE_DIR}
)


# Link paths
link_directories( ${WEBOTS_LIB_DIR} )

# Source files
set(SOURCE_FILES
	CozmoWorldComms.cpp
	world_comms_controller.cpp
	${COZMO_SRC_DIR}/shared/comms/TcpSocket/TcpServer.cpp
)

set(HEADER_FILES
	CozmoWorldComms.h
	${COZMO_SRC_DIR}/shared/comms/TcpSocket/TcpServer.h
)

add_executable(world_comms_controller ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(world_comms_controller CppController)

set_target_properties(world_comms_controller PROPERTIES
	XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++98"
	XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libstdc++"
)

# Make sure a copy of the target exists	     in cozmo_c_controller/
# no matter where the target is	 actually built.
add_custom_command(
	TARGET world_comms_controller
    POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:world_comms_controller> ${CMAKE_CURRENT_SOURCE_DIR}/
	COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:world_comms_controller>
)
