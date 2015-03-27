# products-cozmo/robot/simulator/controllers/cozmo_viz_controller/CMakeLists.txt

# Include paths
  include_directories(
  ${WEBOTS_CPP_INCLUDE_DIR}
)

# Link paths
link_directories( ${WEBOTS_LIB_DIR} )

# Source files
set(SOURCE_FILES
  cozmo_viz_controller.cpp
)

set(HEADER_FILES

)

add_executable(cozmo_viz_controller 
  ${SOURCE_FILES} 
  ${HEADER_FILES}
)

target_link_libraries(cozmo_viz_controller 
  CoreTech_Messaging_Basestation
  CoreTech_Vision_Basestation
  CppController # Webots C++ Controller
  ${OPENCV_LIBS}
)

set_target_properties(cozmo_viz_controller PROPERTIES
    FOLDER "Webots Controllers"
)

# Make sure a copy of the target exists  in cozmo_viz_controller/
# no matter where the target is actually built.
add_custom_command(
  TARGET cozmo_viz_controller
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cozmo_viz_controller> ${CMAKE_CURRENT_SOURCE_DIR}/
#  COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:cozmo_viz_controller>
)

# Copy into cozmo-game simulator controllers also
if (COZMO_GAME_SIMULATOR_DIR)
add_custom_command(
  TARGET cozmo_viz_controller
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${COZMO_GAME_SIMULATOR_DIR}/controllers/cozmo_viz_controller/
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cozmo_viz_controller> ${COZMO_GAME_SIMULATOR_DIR}/controllers/cozmo_viz_controller/
)  
endif (COZMO_GAME_SIMULATOR_DIR)  
