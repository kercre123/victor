# Overlay display for tracing a path of where the robot has gone.
# KEVIN: May remove this later in favor of drawing paths through the physics plugin.

cmake_minimum_required(VERSION 2.8)

project(overlay_display_supervisor)

# Include paths
set(WEBOTS_CPP_INCLUDE_DIR /Applications/Webots/include/controller/c)
include_directories(${WEBOTS_CPP_INCLUDE_DIR})


# Link paths
set(WEBOTS_LIB_DIR /Applications/Webots/lib)
link_directories( ${WEBOTS_LIB_DIR} )


# Source files
set(SOURCE_FILES
	overlay_display_supervisor.c
)


add_executable(overlay_display_supervisor ${SOURCE_FILES} )

target_link_libraries(overlay_display_supervisor Controller)

# Make sure a copy of the target exists		 in cozmo_c_controller/
# no matter where the target is	 actually built.
add_custom_command(
	TARGET overlay_display_supervisor
        POST_BUILD
	COMMAND cp overlay_display_supervisor ${CMAKE_CURRENT_SOURCE_DIR}/
)
