# products-cozmo/robot/simulator/controllers/blockworld_controller/CMakeLists.txt

# Include paths
  include_directories(
#  ${WEBOTS_C_INCLUDE_DIR}
  ${WEBOTS_CPP_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/basestation/src
)


# Link paths
link_directories( ${WEBOTS_LIB_DIR} )

# TODO: I should link against coretech/messaging instead of pulling source directly!

# Source files
set(SOURCE_FILES
#	CozmoWorldComms.cpp
	blockworld_controller.cpp

  ${PROJECT_SOURCE_DIR}/basestation/src/messages.cpp

#	${PROJECT_SOURCE_DIR}/coretech/messaging/robot/TcpSocket/TcpServer.cpp
)

set(HEADER_FILES
	CozmoWorldComms.h
#	${PROJECT_SOURCE_DIR}/coretech/messaging/include/anki/messaging/TcpServer.h
)

if(NOT WIN32)
  add_executable(blockworld_controller 
    ${SOURCE_FILES} 
    ${HEADER_FILES}
  )

  target_link_libraries(blockworld_controller 
    Cozmo_Basestation
    CoreTech_Common_Basestation
    CoreTech_Vision_Basestation
  # Controller # Webots C Controller
    CppController # Webots C++ Controller
    ${OPENCV_LIBS}
  )
endif()
  
#set_target_properties(blockworld_controller PROPERTIES
#  COMPILE_DEFINITIONS USE_TCP_MESSAGING_INTERFACE
#	XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++98"
#   XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libstdc++"
#)

# Make sure a copy of the target exists	in cozmo_c_controller/
# no matter where the target is actually built.
add_custom_command(
	TARGET blockworld_controller
    POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:blockworld_controller> ${CMAKE_CURRENT_SOURCE_DIR}/
	COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:blockworld_controller>
)
