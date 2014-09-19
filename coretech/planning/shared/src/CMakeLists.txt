# coretech/planning/shared/src/CMakeLists.txt

# Shared Planning Library

file(GLOB SOURCE_FILES "*.cpp")

file(GLOB HEADER_FILES 
	"*.h"
	"${CORETECH_PLANNING_INCLUDE_DIR}/shared/*.h"
)

add_library(CoreTech_Planning_Shared 
  ${SOURCE_FILES} 
  ${HEADER_FILES}
  ${CORETECH_PLANNING_INCLUDE_FILES}
)
