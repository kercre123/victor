# coretech/messaging/basestation/src/CMakeLists.txt

# Basestation Messaging Library

file(GLOB SOURCE_FILES "*.cpp")

file(GLOB HEADER_FILES 
	"*.h"
	"${CORETECH_MESSAGING_INCLUDE_DIR}/basestation/*.h"
)

if(NOT WIN32)
  add_library(CoreTech_Messaging_Basestation 
    ${SOURCE_FILES} 
    ${HEADER_FILES}
    ${CORETECH_MESSAGING_INCLUDE_FILES}  
  )
endif()


