# coretech/planning/CMakeLists.txt

set(CORETECH_PLANNING_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/anki/planning)
file(GLOB CORETECH_PLANNING_INCLUDE_FILES "${CORETECH_PLANNING_INCLUDE_DIR}/*.h" "${CORETECH_PLANNING_INCLUDE_DIR}/shared/*.h")

set(CORETECH_PLANNING_SHARED_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shared/src)
file(GLOB CORETECH_PLANNING_SHARED_SRC_FILES "${CORETECH_PLANNING_SHARED_SRC_DIR}/*.cpp")

if(APPLE)
  add_subdirectory(basestation)

  if(NOT ANKI_IOS_BUILD)
    # add_subdirectory(shared/src)
    # add_subdirectory(shared/test)

    add_subdirectory(robot/src)
    # add_subdirectory(robot/test) 

    # add_subdirectory(matlab/mex)
    # add_subdirectory(matlab/CameraCapture)
  endif()
endif()
