# Controller for advertisement service when using simulated robots

cmake_minimum_required(VERSION 2.8)

# Include paths
file(GLOB COZMO_ROBOT_HEADER_FILES "${COZMO_INCLUDE_DIR}/anki/cozmo/robot/*.h")

include_directories(
  ${WEBOTS_CPP_INCLUDE_DIR}
)

# Link paths
link_directories( ${WEBOTS_LIB_DIR} )

# Source files
set(SOURCE_FILES
  robot_advertisement_controller.cpp
)

set(HEADER_FILES
  ${COZMO_ROBOT_HEADER_FILES}
  ${COZMO_INCLUDE_FILES}
)

add_executable(robot_advertisement_controller ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(robot_advertisement_controller 
  CppController # Webots C++ Controller
  CoreTech_Common_Robot
  CoreTech_Messaging_Robot
)
  
set_target_properties(robot_advertisement_controller PROPERTIES
    FOLDER "Webots Controllers"
)

# Make sure a copy of the target exists in robot_advertisement_controller/
# no matter where the target is actually built.
add_custom_command(
  TARGET robot_advertisement_controller
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:robot_advertisement_controller> ${CMAKE_CURRENT_SOURCE_DIR}/
  COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:robot_advertisement_controller>
)
