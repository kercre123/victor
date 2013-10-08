# Cozmo C++-based controller for Webots simulator
# 
# Environment variable MATLAB_APP_ROOT needs to be defined!

cmake_minimum_required(VERSION 2.8)

project(cozmo_c_controller)

# Include paths
set(COZMO_SRC_DIR ../../../src)
set(COZMO_ROBOT_SRC_DIR ${COZMO_SRC_DIR}/robot)
set(COZMO_PHYSICS_PLUGIN_DIR ../../plugins/physics/cozmo_physics)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
	${COZMO_ROBOT_SRC_DIR}
	${COZMO_SRC_DIR}/shared
	${COZMO_SRC_DIR}/util
	${COZMO_PHYSICS_PLUGIN_DIR}
    ${WEBOTS_CPP_INCLUDE_DIR}
)

# Link paths
link_directories( ${WEBOTS_LIB_DIR} )

# NEed to get Cmake to do this somehow...
#-L/Applications/Webots/lib/ -lCppController -L../../../coretech-common/build/xcode4/lib/ -lCoreTech_Common_Embedded_64Debug -lCoreTech_Common_64Debug -L/Applications/MATLAB_R2012b.app/bin/maci64/ -lmx -leng -Wl,-rpath,${MATLAB_APP_ROOT}/bin/maci64/

# Source files
set(SOURCE_FILES
	cozmo_c_controller.cpp
	cozmoBot.cpp
	keyboardController.cpp

	${COZMO_ROBOT_SRC_DIR}/app/mainExecution.cpp
	${COZMO_ROBOT_SRC_DIR}/app/pathFollower.cpp
	${COZMO_ROBOT_SRC_DIR}/app/steeringController.cpp
	${COZMO_ROBOT_SRC_DIR}/app/vehicleSpeedController.cpp
	${COZMO_ROBOT_SRC_DIR}/app/wheelController.cpp
	${COZMO_ROBOT_SRC_DIR}/app/vehicleMath.cpp
	${COZMO_SRC_DIR}/util/utilMessaging.c

	${COZMO_ROBOT_SRC_DIR}/hal/simhal/sim_encoders.cpp
	${COZMO_ROBOT_SRC_DIR}/app/simapp/sim_localization.cpp
	${COZMO_ROBOT_SRC_DIR}/hal/simhal/sim_motors.cpp
	${COZMO_ROBOT_SRC_DIR}/hal/simhal/sim_timers.cpp
)

set(HEADER_FILES
	cozmoBot.h
	keyboardController.h
	${COZMO_ROBOT_SRC_DIR}/cozmoConfig.h
	${COZMO_ROBOT_SRC_DIR}/cozmoTypes.h

	${COZMO_ROBOT_SRC_DIR}/app/assert.h
	${COZMO_ROBOT_SRC_DIR}/app/debug.h
	${COZMO_ROBOT_SRC_DIR}/hal/hal.h
	${COZMO_ROBOT_SRC_DIR}/app/mainExecution.h
	${COZMO_ROBOT_SRC_DIR}/app/pathFollower.h
	${COZMO_ROBOT_SRC_DIR}/app/steeringController.h
	${COZMO_ROBOT_SRC_DIR}/app/vehicleSpeedController.h
	${COZMO_ROBOT_SRC_DIR}/app/wheelController.h
	${COZMO_ROBOT_SRC_DIR}/app/vehicleMath.h
	${COZMO_ROBOT_SRC_DIR}/app/localization.h
	${COZMO_SRC_DIR}/util/utilMessaging.h
	${COZMO_ROBOT_SRC_DIR}/app/trace.h

	${COZMO_SRC_DIR}/shared/comms/cozmoMsgProtocol.h

)

add_executable(cozmo_c_controller ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(cozmo_c_controller 
	CppController
	CoreTech_Common_Embedded
	CoreTech_Common
	mx 
	eng
)

set_target_properties(
	cozmo_c_controller PROPERTIES
	LINK_FLAGS "-Wl,-rpath,${MATLAB_ROOT_DIR}/bin/maci64/"
	XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++98"
	XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libstdc++"
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
