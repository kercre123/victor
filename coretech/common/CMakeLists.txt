# coretech/common/CMakeLists.txt

set(CORETECH_COMMON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/anki/common)

file(GLOB CORETECH_COMMON_INCLUDE_FILES ${CORETECH_COMMON_INCLUDE_DIR}/*.h)

add_subdirectory(basestation/src)
add_subdirectory(basestation/test)

add_subdirectory(robot/src)
add_subdirectory(robot/mex)

add_subdirectory(matlab/mex)

#if(WIN32)
add_subdirectory(tools/communication)
#endif()
