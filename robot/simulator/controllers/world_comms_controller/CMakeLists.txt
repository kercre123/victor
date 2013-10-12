# products-cozmo/robot/simulator/controllers/world_comms_controller/CMakeLists.txt

# Include paths
include_directories(${WEBOTS_CPP_INCLUDE_DIR})


# Link paths
link_directories( ${WEBOTS_LIB_DIR} )

# TODO: I should link against coretech/messaging instead of pulling source directly!

# Source files
set(SOURCE_FILES
	CozmoWorldComms.cpp
	world_comms_controller.cpp
	${PROJECT_SOURCE_DIR}/coretech/messaging/robot/TcpSocket/TcpServer.cpp
)

set(HEADER_FILES
	CozmoWorldComms.h
	${PROJECT_SOURCE_DIR}/coretech/messaging/robot/TcpSocket/TcpServer.h
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
