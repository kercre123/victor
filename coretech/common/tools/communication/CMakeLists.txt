# coretech/common/robot/tools/CMakeLists.txt

# save buffered video

file(GLOB SOURCE_FILES_SAVE_VIDEO
  run_pc_saveVideo.cpp
  serial.cpp
  messageHandling_save.cpp
)

file(GLOB SOURCE_FILES_DISPLAY_VIDEO
  run_pc_displayVideo.cpp
  serial.cpp
  messageHandling_display.cpp
)

file(GLOB SOURCE_FILES_TERMINAL
  run_pc_terminal.cpp
  serial.cpp
)

file(GLOB HEADER_FILES
	serial.h
  threadSafeQueue.h
  messageHandling.h
  C:/Anki/products-cozmo/basestation/src/messageDefMacros.h
)

add_executable(run_pc_saveVideo
	${SOURCE_FILES_SAVE_VIDEO}
	${HEADER_FILES})

target_link_libraries(run_pc_saveVideo
	CoreTech_Common_Robot
  CoreTech_Vision_Robot
	${ALL_EMBEDDED_LIBRARIES})

add_executable(run_pc_displayVideo
	${SOURCE_FILES_DISPLAY_VIDEO}
	${HEADER_FILES})

target_link_libraries(run_pc_displayVideo
	CoreTech_Common_Robot
  CoreTech_Vision_Robot
	${ALL_EMBEDDED_LIBRARIES})  
  
add_executable(run_pc_terminal
	${SOURCE_FILES_TERMINAL}
	${HEADER_FILES})

target_link_libraries(run_pc_terminal
	CoreTech_Common_Robot
  CoreTech_Vision_Robot
	${ALL_EMBEDDED_LIBRARIES})
