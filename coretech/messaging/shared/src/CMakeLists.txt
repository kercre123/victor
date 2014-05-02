# coretech/messaging/robot/src/CMakeLists.txt

# Robot messaging library

file(GLOB SOURCE_FILES "*.c*")

file(GLOB HEADER_FILES
        "*.h"
        "${CORETECH_MESSAGING_INCLUDE_DIR}/shared/*.h"
)

if(NOT WIN32)
  add_library(CoreTech_Messaging_Shared
    ${SOURCE_FILES}
    ${HEADER_FILES}
    ${CORETECH_MESSAGING_INCLUDE_FILES}
  )
  
  set_target_properties(CoreTech_Messaging_Shared PROPERTIES
    FOLDER "CoreTech"#/Shared"
  )
endif()

