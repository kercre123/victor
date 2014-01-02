# coretech/vision/robot/src/CMakeLists.txt

# Robot Shave Vision Library

file(GLOB SOURCE_FILES *.c *.cpp)

file(GLOB HEADER_FILES 
	"*.h"
	"${CORETECH_VISION_INCLUDE_DIR}/robot/shaveKernels_vision_c.h"
)

add_library(CoreTech_Vision_Robot_Shave 
  ${SOURCE_FILES} 
  ${HEADER_FILES}
  ${CORETECH_VISION_INCLUDE_FILES}  
)

