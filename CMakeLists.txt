cmake_minimum_required(VERSION 2.8)

project(cozmo_simulator)

set(COZMO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

# TODO: Get this from an environment variable?
set(WEBOTS_APP_DIR /Applications/Webots)
set(WEBOTS_CPP_INCLUDE_DIR ${WEBOTS_APP_DIR}/include/controller/cpp)
set(WEBOTS_C_INCLUDE_DIR ${WEBOTS_APP_DIR}/include/controller/c)
set(WEBOTS_LIB_DIR ${WEBOTS_APP_DIR}/lib)

add_subdirectory(plugins/physics/cozmo_physics)
add_subdirectory(controllers/overlay_display_supervisor)
add_subdirectory(controllers/world_comms_controller)
add_subdirectory(controllers/cozmo_c_controller)
