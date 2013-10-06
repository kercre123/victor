# Cozmo C++-based controller for Webots simulator
# 
# Environment variable MATLAB_APP_ROOT needs to be defined!

cmake_minimum_required(VERSION 2.8)

project(cozmo_c_controller)

#Import environment variables
set(MATLAB_APP_ROOT $ENV{MATLAB_APP_ROOT})

# Include paths
set(COZMO_SRC_DIR ../../../src)
set(COZMO_ROBOT_SRC_DIR ${COZMO_SRC_DIR}/robot)
set(COZMO_PHYSICS_PLUGIN_DIR ../../plugins/physics/cozmo_physics)
set(WEBOTS_CPP_INCLUDE_DIR /Applications/Webots/include/controller/cpp)
set(MATLAB_INCLUDE_DIR ${MATLAB_APP_ROOT}/extern/include)
set(CORETECH_COMMON_INCLUDE ../../../../coretech-common/include)

set(OPENCV_ROOT ../../../../coretech-external/opencv-2.4.6.1)
set(OPENCV_INCLUDE_DIRS 
			${OPENCV_ROOT}/include
			${OPENCV_ROOT}/modules/calib3d/include
			${OPENCV_ROOT}/modules/contrib/include
			${OPENCV_ROOT}/modules/core/include
			${OPENCV_ROOT}/modules/features2d/include
			${OPENCV_ROOT}/modules/flann/include
			${OPENCV_ROOT}/modules/highgui/include
			${OPENCV_ROOT}/modules/imgproc/include
			${OPENCV_ROOT}/modules/ml/include
			${OPENCV_ROOT}/modules/objdetect/include
			${OPENCV_ROOT}/modules/photo/include
			${OPENCV_ROOT}/modules/video/include
)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
		     ${COZMO_ROBOT_SRC_DIR}
                     ${COZMO_SRC_DIR}/shared
		     ${COZMO_SRC_DIR}/util
		     ${COZMO_PHYSICS_PLUGIN_DIR}
                     ${WEBOTS_CPP_INCLUDE_DIR}
		     ${MATLAB_INCLUDE_DIR}
		     ${CORETECH_COMMON_INCLUDE}
		     ${OPENCV_INCLUDE_DIRS}
)


# Link paths
set(WEBOTS_LIB_DIR /Applications/Webots/lib)
set(MATLAB_LIB_DIR ${MATLAB_APP_ROOT}/bin/maci64)
set(CORETECH_COMMON_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../coretech-common/build/xcode4/lib)
link_directories( ${WEBOTS_LIB_DIR} 
		  ${MATLAB_LIB_DIR}
		  ${CORETECH_COMMON_LIB_DIR}
)


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
		      CoreTech_Common_Embedded_64Debug 
		      CoreTech_Common_64Debug
		      mx 
		      eng
)

set_target_properties(
	cozmo_c_controller PROPERTIES
	LINK_FLAGS "-Wl,-rpath,${MATLAB_APP_ROOT}/bin/maci64/"
)

# Make sure a copy of the target exists in cozmo_c_controller/
# no matter where the target is actually built.
add_custom_command(
	TARGET cozmo_c_controller
	POST_BUILD
	COMMAND cp cozmo_c_controller ${CMAKE_CURRENT_SOURCE_DIR}/
)
