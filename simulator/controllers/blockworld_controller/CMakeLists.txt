# products-cozmo/robot/simulator/controllers/blockworld_controller/CMakeLists.txt

remove_definitions(-DCOZMO_ROBOT -DSIMULATOR)
add_definitions(-DCOZMO_BASESTATION)

# Include paths
  include_directories(
  ${WEBOTS_CPP_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/robot/simulator/src

  ${PROJECT_SOURCE_DIR}/basestation/src/BLEManager
  ${PROJECT_SOURCE_DIR}/basestation/src/BLEManager/Central
  ${PROJECT_SOURCE_DIR}/basestation/src/BLEManager/Networking
  ${PROJECT_SOURCE_DIR}/basestation/src/BLEManager/Logging
  ${PROJECT_SOURCE_DIR}/basestation/src/BLEManager/VehicleManager
)

file(GLOB COZMO_BASESTATION_OBJC_SRC ${PROJECT_SOURCE_DIR}/basestation/src/*.m*)


# Link paths
link_directories( ${WEBOTS_LIB_DIR} )


# Source files
set(SOURCE_FILES
  blockworld_controller.cpp
  ${COZMO_BASESTATION_OBJC_SRC}
)

# Enable ARC
add_compile_options(-fobjc-arc)

find_library(BLE_LIBRARY NAMES IOBluetooth)

add_executable(blockworld_controller 
  ${SOURCE_FILES} 
)

target_link_libraries(blockworld_controller 
  Cozmo_Basestation
  Cozmo_Game
  CoreTech_Common_Basestation
  CoreTech_Vision_Basestation
  CoreTech_Vision_Robot  # For using embedded vision implementations on Basestation
  CoreTech_Common_Robot  #     "
  CoreTech_Planning_Basestation
  CoreTech_Messaging_Basestation
  CppController # Webots C++ Controller
  jsoncpp
  ${OPENCV_LIBS}
  ${BLE_LIBRARY}
)

if(${ANKICORETECH_USE_MATLAB})
  target_link_libraries(blockworld_controller
    ${MATLAB_ENG_LIBRARY}
    ${MATLAB_MX_LIBRARY}
  )
endif()

set_target_properties(blockworld_controller PROPERTIES
    FOLDER "Webots Controllers"
)
#  COMPILE_DEFINITIONS USE_TCP_MESSAGING_INTERFACE
#  XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++98"
#   XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libstdc++"
#)

# Make sure a copy of the target exists  in cozmo_c_controller/
# no matter where the target is actually built.
add_custom_command(
  TARGET blockworld_controller
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:blockworld_controller> ${CMAKE_CURRENT_SOURCE_DIR}/
  COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:blockworld_controller>
)
