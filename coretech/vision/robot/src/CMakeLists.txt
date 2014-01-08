# coretech/vision/robot/src/CMakeLists.txt

# Robot Vision Library

file(GLOB SOURCE_FILES *.c *.cpp shave/*.c)

file(GLOB HEADER_FILES 
	"*.h"
	"${CORETECH_VISION_INCLUDE_DIR}/robot/*.h"
)

add_library(CoreTech_Vision_Robot 
  ${SOURCE_FILES} 
  ${HEADER_FILES}
  ${CORETECH_VISION_INCLUDE_FILES}  
)

