# products-cozmo/src/CMakeLists.txt

set(SOURCE_FILES
	block.cpp
	blockWorld.cpp
	cozmo.cpp
	mat.cpp
	robot.cpp
)

set(HEADER_FILES
	../include/anki/cozmo.h
	../include/anki/cozmo/block.h
	../include/anki/cozmo/blockWorld.h
	../include/anki/cozmo/mat.h
	../include/anki/cozmo/robot.h
)

add_library(Cozmo ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(Cozmo CoreTech_Common CoreTech_Vision z)
