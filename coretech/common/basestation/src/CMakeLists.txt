# coretech/common/basestation/src/CMakeLists.txt

add_definitions(-DCORETECH_BASESTATION)

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB SHARED_SOURCE_FILES ../../shared/src/*.cpp)

file(GLOB_RECURSE PRIVATE_HEADER_FILES *.h)
file(GLOB_RECURSE HEADER_FILES ${CORETECH_COMMON_INCLUDE_DIR}/basestation/ *.h)

file(GLOB SHARED_HEADER_FILES 
  ../../shared/src/*.h
  ${CORETECH_COMMON_INCLUDE_DIR}/shared/*.h
)

add_library(CoreTech_Common_Basestation
  ${SOURCE_FILES}
  ${SHARED_SOURCE_FILES}
  ${HEADER_FILES}
  ${PRIVATE_HEADER_FILES}
  ${SHARED_HEADER_FILES}
  ${CORETECH_COMMON_INCLUDE_FILES}
)
    
set_target_properties(CoreTech_Common_Basestation PROPERTIES
  FOLDER "CoreTech"#/Basestation"
)



