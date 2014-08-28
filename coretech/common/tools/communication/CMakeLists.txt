# coretech/common/robot/tools/CMakeLists.txt

if(LINUX==0)

  # save buffered video

  file(GLOB SOURCE_FILES_SAVE_VIDEO
    run_pc_saveVideo.cpp
  )

  file(GLOB SOURCE_FILES_DISPLAY_VIDEO
    run_pc_displayVideo.cpp
  )

  file(GLOB SOURCE_FILES_DEBUG_STREAM_PARSER
    communication.cpp
    debugStreamClient.cpp
  )

  file(GLOB HEADER_FILES
    communication.h
    ../threads/anki/tools/threads/threadSafeUtilities.h
    ../threads/anki/tools/threads/threadSafeQueue.h
    debugStreamClient.h
    C:/Anki/products-cozmo/basestation/src/messageDefMacros.h
  )

  #
  # run_pc_saveVideo
  #

  add_executable(run_pc_saveVideo
    ${SOURCE_FILES_SAVE_VIDEO}
    ${HEADER_FILES})

  target_link_libraries(run_pc_saveVideo
    CoreTech_Common_Robot
    CoreTech_Vision_Robot
    DebugStreamClient
    ${ALL_EMBEDDED_LIBRARIES})

  set_target_properties(run_pc_saveVideo PROPERTIES
    FOLDER "Tools"
  )

  #
  # run_pc_displayVideo
  #

  add_executable(run_pc_displayVideo
    ${SOURCE_FILES_DISPLAY_VIDEO}
    ${HEADER_FILES})

  target_link_libraries(run_pc_displayVideo
    CoreTech_Common_Robot
    CoreTech_Vision_Robot
    DebugStreamClient
    ${ALL_EMBEDDED_LIBRARIES})

  set_target_properties(run_pc_displayVideo PROPERTIES
    FOLDER "Tools"
  )

  #
  # DebugStreamClient
  #

  add_definitions(-DCORETECH_ROBOT)

  add_library(DebugStreamClient
    ${SOURCE_FILES_DEBUG_STREAM_PARSER}
    ${HEADER_FILES}
  )

  set_target_properties(DebugStreamClient PROPERTIES
    FOLDER "Tools"
  )

endif()