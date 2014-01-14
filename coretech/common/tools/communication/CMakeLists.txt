# coretech/common/robot/tools/CMakeLists.txt

# save buffered video

file(GLOB SOURCE_FILES_SAVE_VIDEO
  run_pc_saveVideo.cpp
  serial.cpp
)

file(GLOB SOURCE_FILES_TERMINAL
  run_pc_terminal.cpp
  serial.cpp
)

file(GLOB HEADER_FILES 
	serial.h
  threadSafeQueue.h
  C:/Anki/products-cozmo/basestation/src/messageDefMacros.h
)

add_executable(run_pc_saveVideo 
	${SOURCE_FILES_SAVE_VIDEO} 
	${HEADER_FILES})

target_link_libraries(run_pc_saveVideo 
	CoreTech_Common_Robot 
	gtest ${OPENCV_LIBS} 
	${MATLAB_ENG_LIBRARY} ${MATLAB_MX_LIBRARY} ${ZLIB_LIBRARY})

add_executable(run_pc_terminal 
	${SOURCE_FILES_TERMINAL} 
	${HEADER_FILES})

target_link_libraries(run_pc_terminal 
	CoreTech_Common_Robot 
	gtest ${OPENCV_LIBS} 
	${MATLAB_ENG_LIBRARY} ${MATLAB_MX_LIBRARY} ${ZLIB_LIBRARY})
