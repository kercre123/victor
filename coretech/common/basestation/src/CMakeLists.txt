# coretech/common/basestation/src/CMakeLists.txt

add_definitions(-DCORETECH_BASESTATION)

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB SHARED_SOURCE_FILES ../../shared/src/*.cpp)

file(GLOB_RECURSE PRIVATE_HEADER_FILES *.h *.hpp)
file(GLOB_RECURSE HEADER_FILES ${CORETECH_COMMON_INCLUDE_DIR}/basestation/*.h ${CORETECH_COMMON_INCLUDE_DIR}/basestation/*.hpp)

if(ANKI_IOS_BUILD)
  file(GLOB OBJC_SOURCE_FILES *.m*)
  list(APPEND SOURCE_FILES ${OBJC_SOURCE_FILES})
endif(ANKI_IOS_BUILD)

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

target_link_libraries(CoreTech_Common_Basestation
  jsoncpp
  ${OPENCV_LIBS}
  util)


