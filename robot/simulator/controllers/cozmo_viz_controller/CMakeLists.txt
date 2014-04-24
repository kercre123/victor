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

if(NOT WIN32)
  add_executable(cozmo_viz_controller 
    ${SOURCE_FILES} 
    ${HEADER_FILES}
  )

  target_link_libraries(cozmo_viz_controller 
    CoreTech_Messaging_Shared
    CppController # Webots C++ Controller
    ${OPENCV_LIBS}
  )
endif()
  
#set_target_properties(blockworld_controller PROPERTIES
#  COMPILE_DEFINITIONS USE_TCP_MESSAGING_INTERFACE
#  XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++98"
#   XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libstdc++"
#)

# Make sure a copy of the target exists  in cozmo_viz_controller/
# no matter where the target is actually built.
add_custom_command(
  TARGET cozmo_viz_controller
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cozmo_viz_controller> ${CMAKE_CURRENT_SOURCE_DIR}/
  COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:cozmo_viz_controller>
)
