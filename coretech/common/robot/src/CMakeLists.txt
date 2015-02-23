# coretech/common/robot/src/CMakeLists.txt

add_definitions(-DCORETECH_ROBOT)

file(GLOB SOURCE_FILES *.c *.cpp ../../shared/src/*.cpp)

file(GLOB HEADER_FILES 
  *.h 
  ../../shared/src/*.h
  ${CORETECH_COMMON_INCLUDE_DIR}/robot/*.h
  ${CORETECH_COMMON_INCLUDE_DIR}/shared/*.h
)

add_library(CoreTech_Common_Robot
  ${SOURCE_FILES} 
  ${HEADER_FILES}
  ${CORETECH_COMMON_INCLUDE_FILES}
)

set_target_properties(CoreTech_Common_Robot PROPERTIES
  FOLDER "CoreTech"#/Robot"
)

target_link_libraries(CoreTech_Common_Robot
  jsoncpp
  ${OPENCV_LIBS})