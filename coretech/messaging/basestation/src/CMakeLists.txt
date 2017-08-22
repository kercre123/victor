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

add_library(cti_messaging
  ${SOURCE_FILES} 
  # ${HEADER_FILES}
  # ${CORETECH_MESSAGING_INCLUDE_FILES}  
)
 
set_target_properties(cti_messaging PROPERTIES
  FOLDER "CoreTech"#/Basestation"
)

target_link_libraries(cti_messaging
  clad
  util
)
