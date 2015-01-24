
if(APPLE)
  # Defines
  add_definitions(-DSIMULATOR)

  # Copy over protos from cozmo-engine
  file(COPY ${COZMO_ENGINE_DIR}/simulator/protos/
       DESTINATION ${PROJECT_SOURCE_DIR}/simulator/protos/CozmoEngineProtos-DO_NOT_MODIFY
       FILE_PERMISSIONS OWNER_READ GROUP_READ
  )

  # TODO: Get this from an environment variable?
  set(WEBOTS_APP_DIR /Applications/Webots)
  set(WEBOTS_CPP_INCLUDE_DIR ${WEBOTS_APP_DIR}/include/controller/cpp)
  set(WEBOTS_C_INCLUDE_DIR ${WEBOTS_APP_DIR}/include/controller/c)
  set(WEBOTS_LIB_DIR ${WEBOTS_APP_DIR}/lib)

  if (IS_DIRECTORY ${WEBOTS_APP_DIR})
    add_subdirectory(controllers/blockworld_controller)
    add_subdirectory(controllers/webots_keyboard_controller)

  else ()
    message(STATUS "Webots app directory not found, not building simulator")
  endif (IS_DIRECTORY ${WEBOTS_APP_DIR})
endif()
