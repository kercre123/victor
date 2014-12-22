# coretech/messaging/CMakeLists.txt

if(APPLE)
  set(CORETECH_MESSAGING_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/anki/messaging)

  file(GLOB CORETECH_MESSAGING_INCLUDE_FILES "${CORETECH_MESSAGING_INCLUDE_DIR}/*.h")

  add_subdirectory(basestation/src)
  
  if(NOT ANKI_IOS_BUILD)
    #add_subdirectory(basestation/test)

    add_subdirectory(robot/src)
    #add_subdirectory(robot/test) 

    #add_subdirectory(shared/src)
  endif()
endif()
