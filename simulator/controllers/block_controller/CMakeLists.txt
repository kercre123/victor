# Block C++-based controller for Webots simulator
 
add_definitions(-DCOZMO_ROBOT)

set(ROBOT_SIM_HAL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/robot/sim_hal)
set(ROBOT_SRC_DIR ${PROJECT_SOURCE_DIR}/robot/supervisor/src)

include_directories( 
  ${COZMO_BASESTATION_INCLUDE_DIR}
  ${ROBOT_SIM_HAL_SOURCE_DIR}
  ${WEBOTS_CPP_INCLUDE_DIR}
)

# Link paths
link_directories( ${WEBOTS_LIB_DIR} )

# Source files
set(SOURCE_FILES
  block_controller.cpp  
  activeBlock.cpp
  ${ROBOT_SIM_HAL_SOURCE_DIR}/BlockMessages.cpp
  ${ROBOT_SRC_DIR}/ledController.cpp
)

set(HEADER_FILES
  activeBlock.h
  ${ROBOT_SIM_HAL_SOURCE_DIR}/BlockMessages.h
  ${ROBOT_SIM_HAL_SOURCE_DIR}/BlockMessageDefinitions.def
)

add_executable(block_controller ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(block_controller 
  CppController
)

set_target_properties(
  block_controller PROPERTIES
  FOLDER "Webots Controllers"
)

# Make sure a copy of the target exists in block_controller/
# no matter where the target is actually built.
add_custom_command(
  TARGET block_controller
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:block_controller> ${CMAKE_CURRENT_SOURCE_DIR}/
)

# Copy into cozmo-game simulator controllers also
if (COZMO_GAME_SIMULATOR_DIR)
add_custom_command(
  TARGET block_controller
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${COZMO_GAME_SIMULATOR_DIR}/controllers/block_controller/
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:block_controller> ${COZMO_GAME_SIMULATOR_DIR}/controllers/block_controller/
)  
endif (COZMO_GAME_SIMULATOR_DIR)  
