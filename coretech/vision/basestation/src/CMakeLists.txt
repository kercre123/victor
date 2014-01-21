# coretech/vision/basestation/src/CMakeLists.txt

# Basestation Vision Library

file(GLOB SOURCE_FILES "*.cpp")

file(GLOB HEADER_FILES 
	"*.h"
	"${CORETECH_VISION_INCLUDE_DIR}/basestation/*.h"
)

if(NOT WIN32)
  add_library(CoreTech_Vision_Basestation 
    ${SOURCE_FILES} 
    ${HEADER_FILES}
    ${CORETECH_VISION_INCLUDE_FILES}  
  )
endif()
  
#target_link_libraries(CoreTech_Vision)


