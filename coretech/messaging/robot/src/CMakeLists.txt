# coretech/messaging/robot/src/CMakeLists.txt

# Robot messaging library

file(GLOB SOURCE_FILES 
	"*.c*"
	"../../shared/src/*.c*"
)

file(GLOB HEADER_FILES
        "*.h"
        "${CORETECH_MESSAGING_INCLUDE_DIR}/robot/*.h"
        "${CORETECH_MESSAGING_INCLUDE_DIR}/shared/*.h"
)

add_library(CoreTech_Messaging_Robot
  ${SOURCE_FILES}
  ${HEADER_FILES}
  ${CORETECH_MESSAGING_INCLUDE_FILES}
)
  
set_target_properties(CoreTech_Messaging_Robot PROPERTIES
  FOLDER "CoreTech"#/Robot"
)
