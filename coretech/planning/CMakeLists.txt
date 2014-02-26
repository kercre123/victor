# coretech/planning/CMakeLists.txt

set(CORETECH_PLANNING_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/anki/planning)

file(GLOB CORETECH_PLANNING_INCLUDE_FILES "${CORETECH_PLANNING_INCLUDE_DIR}/*.h")

add_subdirectory(basestation/src)
add_subdirectory(basestation/test)

add_subdirectory(shared/src)
# add_subdirectory(shared/test)

# add_subdirectory(robot/src)
# add_subdirectory(robot/test) 

# add_subdirectory(matlab/mex)
# add_subdirectory(matlab/CameraCapture)
