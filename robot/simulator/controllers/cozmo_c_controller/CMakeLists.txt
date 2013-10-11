# Cozmo C++-based controller for Webots simulator
# 
# Environment variable MATLAB_APP_ROOT needs to be defined!

cmake_minimum_required(VERSION 2.8)

project(cozmo_c_controller)

# Include paths
set(COZMO_SRC_DIR ../../../src)
set(COZMO_ROBOT_SRC_DIR ${COZMO_SRC_DIR}/robot)
set(COZMO_PHYSICS_PLUGIN_DIR ../../plugins/physics/cozmo_physics)

set(COZMO_INCLUDE_DIR ../../../include)
set(COZMO_ROBOT_INCLUDE_DIR ${COZMO_INCLUDE_DIR}/anki/cozmo/robot)

include_directories( 
	${COZMO_ROBOT_INC_DIR}
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
	keyboardController.cpp

	${COZMO_ROBOT_SRC_DIR}/cozmoBot.cpp
	${COZMO_ROBOT_SRC_DIR}/mainExecution.cpp
	${COZMO_ROBOT_SRC_DIR}/pathFollower.cpp
	${COZMO_ROBOT_SRC_DIR}/steeringController.cpp
	${COZMO_ROBOT_SRC_DIR}/vehicleSpeedController.cpp
	${COZMO_ROBOT_SRC_DIR}/wheelController.cpp
	${COZMO_ROBOT_SRC_DIR}/vehicleMath.cpp
	${COZMO_SRC_DIR}/util/utilMessaging.c

	${COZMO_ROBOT_SRC_DIR}/hal/simhal/sim_encoders.cpp
	${COZMO_ROBOT_SRC_DIR}/app/simapp/sim_localization.cpp
	${COZMO_ROBOT_SRC_DIR}/hal/simhal/sim_motors.cpp
	${COZMO_ROBOT_SRC_DIR}/hal/simhal/sim_timers.cpp
)

set(HEADER_FILES
		
	keyboardController.h

	${COZMO_ROBOT_INCLUDE_DIR}/assert.h
	${COZMO_ROBOT_INCLUDE_DIR}/cozmoBot.h
	${COZMO_ROBOT_INCLUDE_DIR}/cozmoConfig.h
	${COZMO_ROBOT_INCLUDE_DIR}/cozmoTypes.h
	${COZMO_ROBOT_INCLUDE_DIR}/debug.h
	${COZMO_ROBOT_INCLUDE_DIR}/hal.h
	${COZMO_ROBOT_INCLUDE_DIR}/localization.h
	${COZMO_ROBOT_INCLUDE_DIR}/mainExecution.h
	${COZMO_ROBOT_INCLUDE_DIR}/pathFollower.h
	${COZMO_ROBOT_INCLUDE_DIR}/steeringController.h
	${COZMO_ROBOT_INCLUDE_DIR}/trace.h
	${COZMO_ROBOT_INCLUDE_DIR}/vehicleMath.h
	${COZMO_ROBOT_INCLUDE_DIR}/vehicleSpeedController.h
	${COZMO_ROBOT_INCLUDE_DIR}/wheelController.h

	${COZMO_SRC_DIR}/util/utilMessaging.h

	${COZMO_INCLUDE_DIR}/anki/cozmo/cozmoMsgProtocol.h

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
