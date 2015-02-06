# coretech/vision/robot/src/CMakeLists.txt

# Robot Vision Library

add_definitions(-DCORETECH_ROBOT)

file(GLOB SOURCE_FILES *.c *.cpp)

file(GLOB HEADER_FILES 
  "*.h"
  "${CORETECH_VISION_INCLUDE_DIR}/robot/*.h"
)

add_library(CoreTech_Vision_Robot 
  ${SOURCE_FILES} 
  ${HEADER_FILES}
  ${CORETECH_VISION_INCLUDE_FILES}  
)

set_target_properties(CoreTech_Vision_Robot PROPERTIES
  FOLDER "CoreTech"#/Robot"
)

target_link_libraries(CoreTech_Vision_Robot
  CoreTech_Common_Robot)