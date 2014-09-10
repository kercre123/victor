# products-cozmo/robot/simulator/controllers/blockworld_controller/CMakeLists.txt

remove_definitions(-DCOZMO_ROBOT -DSIMULATOR)
add_definitions(-DCOZMO_BASESTATION)

# Include paths
  include_directories(
  ${WEBOTS_CPP_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/basestation/src
  ${PROJECT_SOURCE_DIR}/robot/simulator/src
)

file(GLOB COZMO_BASESTATION_SRC ${PROJECT_SOURCE_DIR}/basestation/src/*.c*)


# Link paths
link_directories( ${WEBOTS_LIB_DIR} )


# Source files
set(SOURCE_FILES
  blockworld_controller.cpp
  ${COZMO_BASESTATION_SRC}
#  ${PROJECT_SOURCE_DIR}/basestation/src/messages.cpp
)

add_executable(blockworld_controller 
  ${SOURCE_FILES} 
)

target_link_libraries(blockworld_controller 
  Cozmo_Basestation
  CoreTech_Common_Basestation
  CoreTech_Vision_Basestation
  CoreTech_Planning_Basestation
  CoreTech_Messaging_Basestation
  CppController # Webots C++ Controller
  jsoncpp
  ${OPENCV_LIBS}
)
  
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
