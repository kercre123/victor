# coretech/messaging/robot/src/CMakeLists.txt

# Robot messaging library

file(GLOB SOURCE_FILES "*.c*")

file(GLOB HEADER_FILES
        "*.h"
        "${CORETECH_MESSAGING_INCLUDE_DIR}/shared/*.h"
)

if(NOT WIN32)
  add_library(CoreTech_Messaging_Robot
    ${SOURCE_FILES}
    ${HEADER_FILES}
    ${CORETECH_MESSAGING_INCLUDE_FILES}
  )
endif()

