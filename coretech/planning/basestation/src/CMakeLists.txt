# coretech/planning/basestation/src/CMakeLists.txt

# Basestation Planning Library

file(GLOB SOURCE_FILES "*.cpp")

file(GLOB HEADER_FILES 
  "*.h"
  "${CORETECH_PLANNING_INCLUDE_DIR}/basestation/*.h"
)

add_library(CoreTech_Planning_Basestation 
  ${SOURCE_FILES}
  ${CORETECH_PLANNING_SHARED_SRC_FILES}
  ${HEADER_FILES}
  ${CORETECH_PLANNING_INCLUDE_FILES}  
)
  
set_target_properties(CoreTech_Planning_Basestation PROPERTIES
  FOLDER "CoreTech"#/Basestation"
)
