# coretech/planning/robot/src/CMakeLists.txt

# Robot Planning Library

add_definitions(-DCORETECH_ROBOT)

file(GLOB SOURCE_FILES "*.cpp")

file(GLOB HEADER_FILES 
  "*.h"
  "${CORETECH_PLANNING_INCLUDE_DIR}/robot/*.h"
)

add_library(CoreTech_Planning_Robot
  ${SOURCE_FILES}
  ${CORETECH_PLANNING_SHARED_SRC_FILES}
  ${HEADER_FILES}
  ${CORETECH_PLANNING_INCLUDE_FILES}  
)
  
set_target_properties(CoreTech_Planning_Robot PROPERTIES
  FOLDER "CoreTech"#/Robot"
)

target_link_libraries(CoreTech_Planning_Robot
  CoreTech_Common_Robot)