# Overlay display for tracing a path of where the robot has gone.
# KEVIN: May remove this later in favor of drawing paths through the physics plugin.

cmake_minimum_required(VERSION 2.8)

# Include paths
include_directories(
  ${WEBOTS_C_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/robot/simulator/src
)

# Link paths
link_directories( ${WEBOTS_LIB_DIR} )

# Source files
set(SOURCE_FILES
  overlay_display_supervisor.c
)

add_executable(overlay_display_supervisor ${SOURCE_FILES} )

target_link_libraries(overlay_display_supervisor Controller)
  
set_target_properties(overlay_display_supervisor PROPERTIES     
  FOLDER "Webots Controllers"
)
  
# Make sure a copy of the target exists     in cozmo_c_controller/
# no matter where the target is   actually built.
add_custom_command(
  TARGET overlay_display_supervisor
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:overlay_display_supervisor> ${CMAKE_CURRENT_SOURCE_DIR}/
  COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:overlay_display_supervisor>
)
