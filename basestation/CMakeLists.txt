# products-cozmo/src/basestation/CMakeLists.txt

set(SOURCE_FILES
	block.cpp
	blockWorld.cpp
	mat.cpp
	robot.cpp
)

set(HEADER_FILES
	../../../include/anki/cozmo.h
	../../../include/anki/cozmo/basestation/block.h
	../../../include/anki/cozmo/basestation/blockWorld.h
	../../../include/anki/cozmo/basestation/mat.h
	../../../include/anki/cozmo/basestation/robot.h
#	../../include/anki/cozmo/dimensions/Block_Dimensions.h
#	../../include/anki/cozmo/dimensions/BlockMarker2d_Dimensions.h
#	../../include/anki/cozmo/dimensions/BlockMarker3d_Dimensions.h
#	../../include/anki/cozmo/dimensions/Robot_Dimensions.h
)

add_library(CozmoBaseStation ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(CozmoBaseStation CoreTech_Common CoreTech_Vision z)
