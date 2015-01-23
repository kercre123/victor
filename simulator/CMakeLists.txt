
if(APPLE)
  # Defines
  add_definitions(-DSIMULATOR)

  # Copy over protos from cozmo-engine
  file(COPY ${COZMO_ENGINE_DIR}/simulator/protos/
       DESTINATION ${PROJECT_SOURCE_DIR}/simulator/protos/CozmoEngineProtos-DO_NOT_MODIFY
       FILE_PERMISSIONS OWNER_READ GROUP_READ
  )

  # Copy over plugins from cozmo-engine
  file(COPY ${COZMO_ENGINE_DIR}/simulator/plugins/physics/cozmo_physics/
       DESTINATION ${PROJECT_SOURCE_DIR}/simulator/plugins/physics/cozmo_physics/
       FILE_PERMISSIONS OWNER_READ GROUP_READ
  )

  # Copy over controllers from cozmo-engine
  #file(COPY ${COZMO_ENGINE_DIR}/simulator/controllers/cozmo_c_controller/
  #     DESTINATION ${PROJECT_SOURCE_DIR}/simulator/controllers/cozmo_c_controller/
  #     FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ
  #)

  # Copy over controllers from cozmo-engine
  file(COPY ${COZMO_ENGINE_DIR}/simulator/controllers/cozmo_viz_controller/
       DESTINATION ${PROJECT_SOURCE_DIR}/simulator/controllers/cozmo_viz_controller/
       FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ
  )



  # TODO: Get this from an environment variable?
  set(WEBOTS_APP_DIR /Applications/Webots)
  set(WEBOTS_CPP_INCLUDE_DIR ${WEBOTS_APP_DIR}/include/controller/cpp)
  set(WEBOTS_C_INCLUDE_DIR ${WEBOTS_APP_DIR}/include/controller/c)
  set(WEBOTS_LIB_DIR ${WEBOTS_APP_DIR}/lib)

  if (IS_DIRECTORY ${WEBOTS_APP_DIR})
    add_subdirectory(plugins/physics/cozmo_physics)
    #add_subdirectory(controllers/overlay_display_supervisor)
    add_subdirectory(controllers/blockworld_controller)
    add_subdirectory(controllers/cozmo_c_controller)
    add_subdirectory(controllers/cozmo_viz_controller)
    add_subdirectory(controllers/webots_keyboard_controller)

  else ()
    message(STATUS "Webots app directory not found, not building simulator")
  endif (IS_DIRECTORY ${WEBOTS_APP_DIR})
endif()
