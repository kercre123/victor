# products-cozmo/robot/simulator/controllers/webots_keyboard_controller/CMakeLists.txt

remove_definitions(-DCOZMO_ROBOT -DSIMULATOR)
add_definitions(-DCOZMO_BASESTATION)



# Include paths
include_directories(
  ${WEBOTS_CPP_INCLUDE_DIR}
)

# Link paths
link_directories( ${WEBOTS_LIB_DIR} )


# Source files
set(SOURCE_FILES
  webots_keyboard_controller.cpp
)

#add_executable(webots_keyboard_controller 
#  ${SOURCE_FILES} 
#)

#target_link_libraries(webots_keyboard_controller 
#  Cozmo_Basestation
#  Cozmo_Game
#  CoreTech_Common_Basestation
#  CoreTech_Messaging_Basestation
#  CppController # Webots C++ Controller
#  jsoncpp
#  SDL2-2.0
#  ${OPENCV_LIBS}
#  ${ALL_EMBEDDED_LIBRARIES}
#)
  
#set_target_properties(webots_keyboard_controller PROPERTIES
#    FOLDER "Webots Controllers"
#)

# Make sure a copy of the target exists  in input_controller/
# no matter where the target is actually built.
#add_custom_command(
#  TARGET webots_keyboard_controller
#  POST_BUILD
#  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:webots_keyboard_controller> ${CMAKE_CURRENT_SOURCE_DIR}/
#)
