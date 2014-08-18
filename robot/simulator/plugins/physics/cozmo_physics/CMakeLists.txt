# Cozmo Webots physics plugin (for drawing robot paths)


# Work in progress...
# Need to CMake to do this somehow!
#-L/Applications/Webots/lib -lode /Applications/Webots/resources/projects/default/plugins/physics/physics.o -flat_namespace -undefined suppress -install_name @executable_path/../../plugins/physics/cozmo_physics/libcozmo_physics.dylib


cmake_minimum_required(VERSION 2.8)

project(cozmo_physics)

# Defines
add_definitions(-DdDOUBLE -DMACOS)

# Include paths
include_directories( ${WEBOTS_APP_DIR}/include/ ${WEBOTS_APP_DIR}/include/ode/ )

set(PHYSICS_O ${WEBOTS_APP_DIR}/resources/projects/default/plugins/physics/physics.o)

set_source_files_properties(
  ${PHYSICS_O} PROPERTIES
  COMPILE_FLAGS -fPIC
  EXTERNAL_OBJECT TRUE  # Identifies this as an object file
  #GENERATED TRUE  # Avoids need for file to exist at configure-time
)


# Link paths
link_directories( ${WEBOTS_LIB_DIR} )

# Source files
set(SOURCE_FILES
  cozmo_physics.cpp
)

set(HEADER_FILES
  cozmo_physics.h
)

add_library(cozmo_physics SHARED ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(cozmo_physics 
  ode 
  ${PHYSICS_O}
  CoreTech_Messaging_Basestation
  CoreTech_Common_Basestation
)

set_target_properties(
  cozmo_physics PROPERTIES
  LINK_FLAGS "-flat_namespace -undefined suppress -install_name @executable_path/../../plugins/physics/cozmo_physics/libcozmo_physics.dylib"
  FOLDER "Webots Controllers"
)

# Make sure a copy of the target exists  in cozmo_c_controller/
# no matter where the target is   actually built.
add_custom_command(
  TARGET cozmo_physics
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cozmo_physics> ${CMAKE_CURRENT_SOURCE_DIR}/
  COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:cozmo_physics>
)
