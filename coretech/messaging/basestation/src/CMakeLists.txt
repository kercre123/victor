# coretech/messaging/basestation/src/CMakeLists.txt

# Basestation Messaging Library

file(GLOB SOURCE_FILES 
	"*.cpp"
	"../../shared/src/*.c*"
)

file(GLOB HEADER_FILES 
  "*.h"
  "${CORETECH_MESSAGING_INCLUDE_DIR}/basestation/*.h"
  "${CORETECH_MESSAGING_INCLUDE_DIR}/shared/*.h"
)

add_library(CoreTech_Messaging_Basestation 
  ${SOURCE_FILES} 
  ${HEADER_FILES}
  ${CORETECH_MESSAGING_INCLUDE_FILES}  
)
 
set_target_properties(CoreTech_Messaging_Basestation PROPERTIES
  FOLDER "CoreTech"#/Basestation"
)
