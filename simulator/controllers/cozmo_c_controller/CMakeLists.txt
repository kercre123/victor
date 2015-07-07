# Cozmo C++-based controller for Webots simulator
# 
# Environment variable MATLAB_ROOT_DIR needs to be defined!

# Include paths
set(SIMULATOR_DIR ${PROJECT_SOURCE_DIR}/simulator)
set(SIMULATOR_SRC_DIR ${SIMULATOR_DIR}/src)
set(SIMULATOR_HAL_DIR ${SIMULATOR_DIR}/hal)
set(PHYSICS_PLUGIN_DIR ${SIMULATOR_DIR}/plugins/physics/cozmo_physics)

set(ROBOT_SRC_DIR ${PROJECT_SOURCE_DIR}/robot/supervisor/src)
set(ROBOT_SIM_HAL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/robot/sim_hal)
set(ROBOT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/robot/include)

file(GLOB COZMO_ROBOT_SUPERVISOR_SRC "${ROBOT_SRC_DIR}/*.cpp")
list(REMOVE_ITEM COZMO_ROBOT_SUPERVISOR_SRC "${ROBOT_SRC_DIR}/offboardVisionSystem.cpp")

file(GLOB COZMO_ROBOT_HEADER_FILES "${COZMO_INCLUDE_DIR}/anki/cozmo/robot/*.h" "${ROBOT_INCLUDE_DIR}/anki/cozmo/robot/*.h")
file(GLOB COZMO_ROBOT_PRIV_HEADER_FILES "${ROBOT_SRC_DIR}/*.h")

file(GLOB COZMO_SIMULATOR_SOURCE_FILES "${SIMULATOR_SRC_DIR}/*.cpp")
file(GLOB COZMO_SIMULATOR_HEADER_FILES "${COZMO_INCLUDE_DIR}/anki/cozmo/simulator/*.h")

include_directories( 
  ${ROBOT_SRC_DIR}
  ${ROBOT_INCLUDE_DIR}
  ${COZMO_INCLUDE_DIR}
  ${COZMO_BASESTATION_INCLUDE_DIR}
  ${PHYSICS_PLUGIN_DIR}
  ${WEBOTS_CPP_INCLUDE_DIR}
)

# Link paths
link_directories( ${WEBOTS_LIB_DIR} )

# Defines
#remove_definitions(-DCOZMO_BASESTATION -DCORETECH_BASESTATION)
add_definitions(-DSIMULATOR -DCOZMO_ROBOT -DCORETECH_ROBOT)

# NEed to get Cmake to do this somehow...
#-L/Applications/Webots/lib/ -lCppController -L../../../coretech-common/build/xcode4/lib/ -lCoreTech_Common_Embedded_64Debug -lCoreTech_Common_64Debug -L/Applications/MATLAB_R2012b.app/bin/maci64/ -lmx -leng -Wl,-rpath,${MATLAB_APP_ROOT}/bin/maci64/

# Source files
set(SOURCE_FILES
  cozmo_c_controller.cpp
  
  # this is where the Supervisor object lives, it must be early in the compile list?!?
  ${ROBOT_SIM_HAL_SOURCE_DIR}/sim_hal.cpp
  ${ROBOT_SIM_HAL_SOURCE_DIR}/sim_radio.cpp
  #${ROBOT_SIM_HAL_SOURCE_DIR}/sim_uart.cpp

  ${ROBOT_SIM_HAL_SOURCE_DIR}/BlockMessages.cpp

  ${COZMO_SIMULATOR_SOURCE_FILES}
  ${COZMO_ROBOT_SUPERVISOR_SRC}
)

source_group(Supervisor FILES ${COZMO_ROBOT_SUPERVISOR_SRC})
source_group(Simulator  REGULAR_EXPRESSION "${SIMULATOR_HAL_DIR}/.*")
source_group(Simulator  REGULAR_EXPRESSION "${SIMULATOR_SRC_DIR}/.*")

set(HEADER_FILES
 ${COZMO_ROBOT_PRIV_HEADER_FILES}
 ${COZMO_ROBOT_HEADER_FILES}
 ${COZMO_INCLUDE_FILES}
 ${COZMO_SIMULATOR_HEADER_FILES}
)

add_executable(cozmo_c_controller ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(cozmo_c_controller 
  CppController
  CoreTech_Common_Robot
  CoreTech_Vision_Robot
  CoreTech_Messaging_Robot
  CoreTech_Planning_Robot
  util
  utilEmbedded
  ${ALL_EMBEDDED_LIBRARIES}
)

set_target_properties(
  cozmo_c_controller PROPERTIES
  COMPILE_DEFINITIONS USE_TCP_MESSAGING_INTERFACE
  LINK_FLAGS "-Wl,-rpath,${MATLAB_ROOT}/bin/maci64/"
  FOLDER "Webots Controllers"
  #XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++98"
  #XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libstdc++"
)

# Make sure a copy of the target exists in cozmo_c_controller/
# no matter where the target is actually built.
add_custom_command(
  TARGET cozmo_c_controller
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cozmo_c_controller> ${CMAKE_CURRENT_SOURCE_DIR}/
#  COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:cozmo_c_controller>
)

# Copy into cozmo-game simulator controllers also
if (COZMO_GAME_SIMULATOR_DIR)
add_custom_command(
  TARGET cozmo_c_controller
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${COZMO_GAME_SIMULATOR_DIR}/controllers/cozmo_c_controller/
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cozmo_c_controller> ${COZMO_GAME_SIMULATOR_DIR}/controllers/cozmo_c_controller/
)  
endif (COZMO_GAME_SIMULATOR_DIR)  
