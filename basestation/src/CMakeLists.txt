# products-cozmo/basestation/src/CMakeLists.txt

file(GLOB_RECURSE SOURCE_FILES "*.cpp")
file(GLOB_RECURSE PRIVATE_HEADER_FILES "*.h")
file(GLOB_RECURSE PUBLIC_HEADER_FILES "${COZMO_INCLUDE_DIR}/anki/cozmo/basestation/*.h")

include_directories(${PROJECT_SOURCE_DIR}/basestation/src)

add_library(Cozmo_Basestation 
  ${SOURCE_FILES} 
  ${PRIVATE_HEADER_FILES}
  ${PUBLIC_HEADER_FILES}
  ${COZMO_HEADER_FILES}
)

target_link_libraries(Cozmo_Basestation 
  CoreTech_Common_Basestation 
  CoreTech_Vision_Basestation 
  CoreTech_Planning_Basestation
  ${ZLIB_LIBRARY}
)
