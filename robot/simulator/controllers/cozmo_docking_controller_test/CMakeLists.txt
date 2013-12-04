# Cozmo C++-based controller for Webots simulator
# 
# Environment variable MATLAB_APP_ROOT needs to be defined!

# Include paths
set(SIMULATOR_DIR ${PROJECT_SOURCE_DIR}/robot/simulator)
set(SIMULATOR_SRC_DIR ${SIMULATOR_DIR}/src)
set(SIMULATOR_HAL_DIR ${SIMULATOR_DIR}/hal)
set(PHYSICS_PLUGIN_DIR ${SIMULATOR_DIR}/plugins/physics/cozmo_physics)

set(ROBOT_SRC_DIR ${PROJECT_SOURCE_DIR}/robot/supervisor/src)


file(GLOB COZMO_ROBOT_HEADER_FILES "${COZMO_INCLUDE_DIR}/anki/cozmo/robot/*.h")

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
	cozmo_docking_controller_test.cpp

  ${SIMULATOR_SRC_DIR}/keyboardController.cpp
  ${SIMULATOR_SRC_DIR}/sim_pathFollower.cpp
  ${SIMULATOR_SRC_DIR}/sim_overlayDisplay.cpp

  ${SIMULATOR_HAL_DIR}/sim_hal.cpp
  ${SIMULATOR_HAL_DIR}/sim_uart.cpp
	
  ${ROBOT_SRC_DIR}/cozmoBot.cpp
  ${ROBOT_SRC_DIR}/dockingController.cpp
  ${ROBOT_SRC_DIR}/commandHandler.cpp
  ${ROBOT_SRC_DIR}/gripController.cpp
  ${ROBOT_SRC_DIR}/headController.cpp
  ${ROBOT_SRC_DIR}/liftController.cpp
  ${ROBOT_SRC_DIR}/localization.cpp
  ${ROBOT_SRC_DIR}/offboardVision.cpp
  ${ROBOT_SRC_DIR}/pathFollower.cpp
  ${ROBOT_SRC_DIR}/steeringController.cpp
  ${ROBOT_SRC_DIR}/speedController.cpp
	${ROBOT_SRC_DIR}/wheelController.cpp
	${ROBOT_SRC_DIR}/visionSystem.cpp
	${ROBOT_SRC_DIR}/testModeController.cpp

	${PROJECT_SOURCE_DIR}/coretech/messaging/robot/utilMessaging.c

#	${COZMO_ROBOT_SRC_DIR}/hal/simhal/sim_encoders.cpp
#	${COZMO_ROBOT_SRC_DIR}/hal/simhal/sim_motors.cpp
#	${COZMO_ROBOT_SRC_DIR}/hal/simhal/sim_timers.cpp
)

set(HEADER_FILES
 ${ROBOT_SRC_DIR}/dockingController.h
 ${ROBOT_SRC_DIR}/gripController.h
 ${ROBOT_SRC_DIR}/headController.h
 ${ROBOT_SRC_DIR}/liftController.h
 ${ROBOT_SRC_DIR}/testModeController.h

	${SIMULATOR_SRC_DIR}/keyboardController.h
  ${SIMULATOR_SRC_DIR}/sim_pathFollower.h
  ${SIMULATOR_SRC_DIR}/sim_overlayDisplay.h
	
	${COZMO_ROBOT_HEADER_FILES}
	${COZMO_INCLUDE_FILES}
)

add_executable(cozmo_docking_controller_test ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(cozmo_docking_controller_test 
	CppController
	CoreTech_Common_Robot
	CoreTech_Common_Basestation # needed?
	mx 
	eng
)

set_target_properties(
	cozmo_docking_controller_test PROPERTIES
  COMPILE_DEFINITIONS USE_TCP_MESSAGING_INTERFACE
	LINK_FLAGS "-Wl,-rpath,${MATLAB_ROOT_DIR}/bin/maci64/"
	#XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++98"
	#XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libstdc++"
)

# Make sure a copy of the target exists in cozmo_c_controller/
# no matter where the target is actually built.
add_custom_command(
	TARGET cozmo_docking_controller_test
	POST_BUILD
	#COMMAND cp cozmo_docking_controller_test ${CMAKE_CURRENT_SOURCE_DIR}/
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cozmo_docking_controller_test> ${CMAKE_CURRENT_SOURCE_DIR}/
	COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:cozmo_docking_controller_test>
)
