
if(APPLE)
  # Defines
  add_definitions(-DSIMULATOR)
  
  include_directories(include)

  # TODO: Get this from an environment variable?
  set(WEBOTS_APP_DIR /Applications/Webots)
  set(WEBOTS_CPP_INCLUDE_DIR ${WEBOTS_APP_DIR}/include/controller/cpp)
  set(WEBOTS_C_INCLUDE_DIR ${WEBOTS_APP_DIR}/include/controller/c)
  set(WEBOTS_LIB_DIR ${WEBOTS_APP_DIR}/lib) 

  if (IS_DIRECTORY ${WEBOTS_APP_DIR})
    add_subdirectory(plugins/physics/cozmo_physics)
    #add_subdirectory(controllers/overlay_display_supervisor)
    #add_subdirectory(controllers/cozmo_c_controller)
    #add_subdirectory(controllers/block_controller)
    #add_subdirectory(controllers/cozmo_viz_controller)

     if (ANKICORETECH_USE_MATLAB)
       add_subdirectory(controllers/visionUnitTestController)
     endif()
  else ()
    message(STATUS "Webots app directory not found, not building simulator")
  endif (IS_DIRECTORY ${WEBOTS_APP_DIR})
endif()
