# products-cozmo/basestation/src/CMakeLists.txt

# root src dir
file(GLOB SOURCE_FILES_ROOT "*.cpp")
file(GLOB HEADER_FILES_ROOT "*.h")

# comms subdirectory
file(GLOB SOURCE_FILES_COMMS "comms/*.cpp")
file(GLOB HEADER_FILES_COMMS "comms/*.h")

# comms/robot subdirectory
file(GLOB SOURCE_FILES_COMMS_ROBOT "comms/robot/*.cpp")
file(GLOB HEADER_FILES_COMMS_ROBOT "comms/robot/*.h")

# game subdirectory
file(GLOB SOURCE_FILES_GAME "game/*.cpp")
file(GLOB HEADER_FILES_GAME "game/*.h")

# events subdirectory
file(GLOB SOURCE_FILES_EVENTS "events/*.cpp")
file(GLOB HEADER_FILES_EVENTS "events/*.h")

# signals subdirectory
file(GLOB SOURCE_FILES_SIGNALS "signals/*.cpp")
file(GLOB HEADER_FILES_SIGNALS "signals/*.h")

# recording subdirectory
file(GLOB SOURCE_FILES_RECORDING "recording/*.cpp")
file(GLOB HEADER_FILES_RECORDING "recording/*.h")

# utils subdirectory
file(GLOB SOURCE_FILES_UTILS "utils/*.cpp")
file(GLOB HEADER_FILES_UTILS "utils/*.h")

# utils/parsingConstants subdirectory
file(GLOB SOURCE_FILES_UTILS_PARSECONSTS "utils/parsingConstants/*.cpp")
file(GLOB HEADER_FILES_UTILS_PARSECONSTS "utils/parsingConstants/*.h")

# BLEManager subdirectory (not on iOS)
#if(NOT ANKI_IOS_BUILD)
#  file(GLOB_RECURSE SOURCE_FILES_BLEMGR "BLEManager/*.m*")
#  file(GLOB_RECURSE HEADER_FILES_BLEMGR "BLEManager/*.h”)
#else()
  set(SOURCE_FILES_BLEMGR )
  set(HEADER_FILES_BLEMGR )
#endif(NOT ANKI_IOS_BUILD)

set(PRIVATE_HEADER_FILES 
  ${HEADER_FILES_ROOT} 
  ${HEADER_FILES_COMMS}
  ${HEADER_FILES_COMMS_ROBOT}
  ${HEADER_FILES_GAME}
  ${HEADER_FILES_EVENTS}
  ${HEADER_FILES_SIGNALS}
  ${HEADER_FILES_RECORDING}
  ${HEADER_FILES_UTILS}
  ${HEADER_FILES_UTILS_PARSECONSTS}
  ${HEADER_FILES_BLEMGR}
)

set(SOURCE_FILES
  ${SOURCE_FILES_ROOT}
  ${SOURCE_FILES_COMMS}
  ${SOURCE_FILES_COMMS_ROBOT}
  ${SOURCE_FILES_GAME}
  ${SOURCE_FILES_EVENTS}
  ${SOURCE_FILES_SIGNALS}
  ${SOURCE_FILES_RECORDING}
  ${SOURCE_FILES_UTILS}
  ${SOURCE_FILES_UTILS_PARSECONSTS}
  ${SOURCE_FILES_BLEMGR}
)

# TODO: Ideally, get rid of this RECURSE as we did above
file(GLOB_RECURSE PUBLIC_HEADER_FILES "${COZMO_INCLUDE_DIR}/anki/cozmo/basestation/*.h")

include_directories(${PROJECT_SOURCE_DIR}/basestation/src
  ${PROJECT_SOURCE_DIR}/basestation/src/BLEManager
  ${PROJECT_SOURCE_DIR}/basestation/src/BLEManager/Central
  ${PROJECT_SOURCE_DIR}/basestation/src/BLEManager/Networking
  ${PROJECT_SOURCE_DIR}/basestation/src/BLEManager/Logging
  ${PROJECT_SOURCE_DIR}/basestation/src/BLEManager/VehicleManager
)

# Enable ARC
#add_compile_options(-fobjc-arc)

add_library(Cozmo_Basestation 
  ${SOURCE_FILES} 
  ${PRIVATE_HEADER_FILES}
  ${PUBLIC_HEADER_FILES}
)

if(NOT ANKI_IOS_BUILD)
  # Grouping source files
  file(GLOB_RECURSE BLE_SOURCE_FILES "BLEManager/*.m*")
  file(GLOB_RECURSE BLE_HEADER_FILES "BLEManager/*.h")
  source_group(BLEManager FILES ${BLE_SOURCE_FILES} ${BLE_HEADER_FILES})
endif()

target_link_libraries(Cozmo_Basestation 
  CoreTech_Common_Basestation 
  CoreTech_Messaging_Basestation 
  CoreTech_Vision_Basestation 
  CoreTech_Planning_Basestation
  CoreTech_Vision_Robot  # TODO: Remove once we do basestation implementation of vision
  CoreTech_Common_Robot  # TODO: Remove once we do basestation implementation of vision
  
  ${ZLIB_LIBRARY}
)

