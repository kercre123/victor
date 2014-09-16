# Cozmo C++-based controller for Webots simulator
# 
# Environment variable MATLAB_APP_ROOT needs to be defined!

# Include paths
set(SIMULATOR_DIR ${PROJECT_SOURCE_DIR}/robot/simulator)
set(SIMULATOR_SRC_DIR ${SIMULATOR_DIR}/src)
set(SIMULATOR_HAL_DIR ${SIMULATOR_DIR}/hal)
set(PHYSICS_PLUGIN_DIR ${SIMULATOR_DIR}/plugins/physics/cozmo_physics)

set(ROBOT_SRC_DIR ${PROJECT_SOURCE_DIR}/robot/supervisor/src)

file(GLOB COZMO_ROBOT_SUPERVISOR_SRC "${ROBOT_SRC_DIR}/*.cpp")

file(GLOB COZMO_ROBOT_HEADER_FILES "${COZMO_INCLUDE_DIR}/anki/cozmo/robot/*.h")
file(GLOB COZMO_ROBOT_PRIV_HEADER_FILES "${ROBOT_SRC_DIR}/*.h")

include_directories( 
  ${ROBOT_SRC_DIR}
  ${SIMULATOR_SRC_DIR}
  ${PHYSICS_PLUGIN_DIR}
  ${WEBOTS_CPP_INCLUDE_DIR}
)

# Link paths
link_directories( ${WEBOTS_LIB_DIR} )

# Defines
add_definitions(-DSIMULATOR)

# NEed to get Cmake to do this somehow...
#-L/Applications/Webots/lib/ -lCppController -L../../../coretech-common/build/xcode4/lib/ -lCoreTech_Common_Embedded_64Debug -lCoreTech_Common_64Debug -L/Applications/MATLAB_R2012b.app/bin/maci64/ -lmx -leng -Wl,-rpath,${MATLAB_APP_ROOT}/bin/maci64/

# Source files
set(SOURCE_FILES
  cozmo_c_controller.cpp
  
  # this is where the Supervisor object lives, it must be early in the compile list?!?
  ${SIMULATOR_HAL_DIR}/sim_hal.cpp
  ${SIMULATOR_HAL_DIR}/sim_radio.cpp
  #${SIMULATOR_HAL_DIR}/sim_uart.cpp

  ${SIMULATOR_SRC_DIR}/keyboardController.cpp
  ${SIMULATOR_SRC_DIR}/sim_viz.cpp
  ${SIMULATOR_SRC_DIR}/sim_overlayDisplay.cpp

  ${COZMO_ROBOT_SUPERVISOR_SRC}
  
#  ${COZMO_ROBOT_SRC_DIR}/hal/simhal/sim_encoders.cpp
#  ${COZMO_ROBOT_SRC_DIR}/hal/simhal/sim_motors.cpp
#  ${COZMO_ROBOT_SRC_DIR}/hal/simhal/sim_timers.cpp
)

source_group(Supervisor FILES ${COZMO_ROBOT_SUPERVISOR_SRC})
source_group(Simulator  REGULAR_EXPRESSION "${SIMULATOR_HAL_DIR}/.*")
source_group(Simulator  REGULAR_EXPRESSION "${SIMULATOR_SRC_DIR}/.*")

set(HEADER_FILES
 ${SIMULATOR_SRC_DIR}/keyboardController.h
 ${SIMULATOR_SRC_DIR}/sim_viz.h
 ${SIMULATOR_SRC_DIR}/sim_overlayDisplay.h
  
 ${COZMO_ROBOT_PRIV_HEADER_FILES}
 ${COZMO_ROBOT_HEADER_FILES}
 ${COZMO_INCLUDE_FILES}
)

add_executable(cozmo_c_controller ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(cozmo_c_controller 
  CppController
  CoreTech_Common_Robot
  CoreTech_Vision_Robot
  CoreTech_Messaging_Robot
  CoreTech_Planning_Robot
  # mx 
  # eng
  ${ALL_EMBEDDED_LIBRARIES}
)

set_target_properties(
  cozmo_c_controller PROPERTIES
  COMPILE_DEFINITIONS USE_TCP_MESSAGING_INTERFACE
  LINK_FLAGS "-Wl,-rpath,${MATLAB_ROOT_DIR}/bin/maci64/"
  FOLDER "Webots Controllers"
  #XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++98"
  #XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libstdc++"
)

# Make sure a copy of the target exists in cozmo_c_controller/
# no matter where the target is actually built.
add_custom_command(
  TARGET cozmo_c_controller
  POST_BUILD
  #COMMAND cp cozmo_c_controller ${CMAKE_CURRENT_SOURCE_DIR}/
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cozmo_c_controller> ${CMAKE_CURRENT_SOURCE_DIR}/
  COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:cozmo_c_controller>
)
