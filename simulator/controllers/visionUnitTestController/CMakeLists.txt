# Cozmo C++-based controller for creating Vision System Unit Tests in Webots

# Override some defines to make this controller compile using basestation options
remove_definitions(-DCOZMO_ROBOT)
add_definitions(-DCOZMO_BASESTATION)

# Include paths
set(SIMULATOR_DIR ${PROJECT_SOURCE_DIR}/robot/simulator)
set(SIMULATOR_SRC_DIR ${SIMULATOR_DIR}/src)
set(SIMULATOR_HAL_DIR ${SIMULATOR_DIR}/hal)
set(PHYSICS_PLUGIN_DIR ${SIMULATOR_DIR}/plugins/physics/cozmo_physics)

set(ROBOT_SRC_DIR ${PROJECT_SOURCE_DIR}/robot/supervisor/src)

file(GLOB COZMO_ROBOT_SUPERVISOR_SRC "${ROBOT_SRC_DIR}/*.cpp")
file(GLOB COZMO_ROBOT_SUPERVISOR_INC "${ROBOT_SRC_DIR}/*.h")

#file(GLOB SIM_HAL_SRC "${SIMULATOR_HAL_DIR}/*.cpp")

file(GLOB COZMO_ROBOT_HEADER_FILES "${COZMO_INCLUDE_DIR}/anki/cozmo/robot/*.h")

include_directories( 
  ${ROBOT_SRC_DIR}
    ${SIMULATOR_SRC_DIR}
  ${PHYSICS_PLUGIN_DIR}
    ${WEBOTS_CPP_INCLUDE_DIR}
)

# Link paths
link_directories( ${WEBOTS_LIB_DIR} )

# Source files
set(SOURCE_FILES
  visionUnitTestController.cpp
  
  ${PROJECT_SOURCE_DIR}/basestation/src/anki/cozmo/basestation/comms/robot/robotMessages.cpp

  # this is where the Supervisor object lives, it must be early in the compile list?!?
  #${SIMULATOR_HAL_DIR}/sim_hal.cpp
  #${SIMULATOR_HAL_DIR}/sim_radio.cpp
  #${SIMULATOR_HAL_DIR}/sim_uart.cpp

  #${SIMULATOR_SRC_DIR}/sim_pathFollower.cpp

  #${SIMULATOR_SRC_DIR}/visionUnitTestCreator.cpp
    
  #${COZMO_ROBOT_SUPERVISOR_SRC}
  #${ROBOT_SRC_DIR}/offboardVision.cpp
  #${ROBOT_SRC_DIR}/messages.cpp
  
)

set(HEADER_FILES
  #  ${COZMO_ROBOT_SUPERVISOR_INC}
  #${COZMO_ROBOT_HEADER_FILES}
  #${COZMO_INCLUDE_FILES}
)

add_executable(visionUnitTestController ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(visionUnitTestController 
  CppController
  CoreTech_Common_Basestation
  #CoreTech_Common_Robot # for matlab interface?
  CoreTech_Vision_Basestation
  CoreTech_Vision_Robot  # needed for marker detection, until that moves to basestation
  #CoreTech_Messaging_Shared
  # mx 
  # eng
  jsoncpp
  ${OPENCV_LIBS}
  #${ALL_EMBEDDED_LIBRARIES}
)

if(ANKICORETECH_USE_MATLAB)
  target_link_libraries(visionUnitTestController
    ${MATLAB_MEX_LIBRARY}
    ${MATLAB_ENG_LIBRARY}
    ${MATLAB_MX_LIBRARY})
endif()

set_target_properties(
  visionUnitTestController PROPERTIES
  COMPILE_DEFINITIONS USE_TCP_MESSAGING_INTERFACE
  LINK_FLAGS "-Wl,-rpath,${MATLAB_ROOT}/bin/maci64/"
  FOLDER "Unit Tests"
  #XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++98"
  #XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libstdc++"
)

# Make sure a copy of the target exists in cozmo_c_controller/
# no matter where the target is actually built.
add_custom_command(
  TARGET visionUnitTestController
  POST_BUILD
  #COMMAND cp visionUnitTestController ${CMAKE_CURRENT_SOURCE_DIR}/
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:visionUnitTestController> ${CMAKE_CURRENT_SOURCE_DIR}/
  COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:visionUnitTestController>
)
