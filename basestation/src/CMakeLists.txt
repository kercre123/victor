# products-cozmo/basestation/src/CMakeLists.txt

file(GLOB_RECURSE SOURCE_FILES "*.cpp")
file(GLOB_RECURSE OBJC_SOURCE_FILES "*.m*")
file(GLOB_RECURSE PRIVATE_HEADER_FILES "*.h")
file(GLOB_RECURSE PUBLIC_HEADER_FILES "${COZMO_INCLUDE_DIR}/anki/cozmo/basestation/*.h")

include_directories(${PROJECT_SOURCE_DIR}/basestation/src
  ${PROJECT_SOURCE_DIR}/basestation/src/BLEManager
  ${PROJECT_SOURCE_DIR}/basestation/src/BLEManager/Central
  ${PROJECT_SOURCE_DIR}/basestation/src/BLEManager/Networking
  ${PROJECT_SOURCE_DIR}/basestation/src/BLEManager/Logging
  ${PROJECT_SOURCE_DIR}/basestation/src/BLEManager/VehicleManager
)

# Enable ARC
add_compile_options(-fobjc-arc)

add_library(Cozmo_Basestation 
  ${SOURCE_FILES} 
  ${OBJC_SOURCE_FILES}
  ${PRIVATE_HEADER_FILES}
  ${PUBLIC_HEADER_FILES}
  ${COZMO_HEADER_FILES}
)

# Grouping source files
file(GLOB_RECURSE BLE_SOURCE_FILES "BLEManager/*.m*")
file(GLOB_RECURSE BLE_HEADER_FILES "BLEManager/*.h")
source_group(BLEManager FILES ${BLE_SOURCE_FILES} ${BLE_HEADER_FILES})


target_link_libraries(Cozmo_Basestation 
  CoreTech_Common_Basestation 
  CoreTech_Vision_Basestation 
  CoreTech_Planning_Basestation
  ${ZLIB_LIBRARY}
)
