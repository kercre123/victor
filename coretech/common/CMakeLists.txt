# coretech/common/CMakeLists.txt

set(CORETECH_COMMON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/anki/common)

file(GLOB CORETECH_COMMON_INCLUDE_FILES ${CORETECH_COMMON_INCLUDE_DIR}/*.h)

if(APPLE)
  add_subdirectory(basestation/src)
  
  if(NOT ANKI_IOS_BUILD)
    add_subdirectory(basestation/test)
  endif()
endif()

# TODO: this can probably be skipped for iOS builds once we aren't using embedded code on Basestation for vision
add_subdirectory(robot/src)

if(NOT ANKI_IOS_BUILD)
  add_subdirectory(robot/mex)
  add_subdirectory(shared/test)
  add_subdirectory(matlab/mex)
  add_subdirectory(tools/communication)
endif()
