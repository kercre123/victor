# coretech/vision/basestation/src/CMakeLists.txt

# Basestation Vision Library

add_definitions(-DCORETECH_BASESTATION)

file(GLOB SOURCE_FILES "*.cpp")

file(GLOB HEADER_FILES 
  "*.h"
  "${CORETECH_VISION_INCLUDE_DIR}/basestation/*.h"
)

add_library(CoreTech_Vision_Basestation 
  ${SOURCE_FILES} 
  ${HEADER_FILES}
  ${CORETECH_VISION_INCLUDE_FILES}  
)
  
set_target_properties(CoreTech_Vision_Basestation PROPERTIES
  FOLDER "CoreTech"#/Basestation"
)
  
#target_link_libraries(CoreTech_Vision)

target_link_libraries(CoreTech_Vision_Basestation
  CoreTech_Common_Basestation)
