# coretech/common/robot/tools/CMakeLists.txt

# save buffered video

file(GLOB SOURCE_FILES 
  *.c 
  *.cpp
)

file(GLOB HEADER_FILES 
	*.h 
)

add_executable(run_pc_saveVideo 
	${SOURCE_FILES} 
	${HEADER_FILES})

target_link_libraries(run_pc_saveVideo 
	CoreTech_Common_Robot 
	gtest ${OPENCV_LIBS} 
	${MATLAB_ENG_LIBRARY} ${MATLAB_MX_LIBRARY} ${ZLIB_LIBRARY})



