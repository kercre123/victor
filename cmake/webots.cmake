#
# cmake/webots.cmake
#
if (NOT MACOSX)
  return()
endif()

# Build options
option(WEBOTS_SETUP_FIREWALL "Set up controller executables for use with webots firewall certificate" ON)

# Path to webotsTest.py
if (NOT DEFINED WEBOTS_TEST)
  set(WEBOTS_TEST "${CMAKE_SOURCE_DIR}/project/build-scripts/webots/webotsTest.py")
endif()

# Webots firewall setup command
if (NOT DEFINED WEBOTS_SETUP)
  set(WEBOTS_SETUP ${WEBOTS_TEST} -g ${CMAKE_GENERATOR} -b ${CMAKE_BUILD_TYPE})
endif()

set(WEBOTS_HOME "/Applications/Webots.app")

set(WEBOTS_INCLUDE_PATHS
   "${WEBOTS_HOME}/include"
   "${WEBOTS_HOME}/include/ode"
   "${WEBOTS_HOME}/include/controller/cpp"
)

set(WEBOTS_LIB_PATH "${WEBOTS_HOME}/lib")

set(WEBOTS_LIBS
    Controller
    CppController
)

set(WEBOTS_SIM_LIBS
    ode
)

set(WEBOTS_LIB_TARGETS
  "${WEBOTS_LIBS}"
  "${WEBOTS_SIM_LIBS}"
)

# Read the Webots version from file
set(WEBOTS_VER_FILE "${WEBOTS_HOME}/resources/version.txt")
file(READ ${WEBOTS_VER_FILE} WEBOTS_VER)
string(STRIP ${WEBOTS_VER} WEBOTS_VER)

foreach(LIB ${WEBOTS_LIB_TARGETS})
    if (NOT TARGET ${LIB})
        add_library(${LIB} SHARED IMPORTED)
        set_target_properties(${LIB} PROPERTIES
            IMPORTED_LOCATION
            "${WEBOTS_LIB_PATH}/lib${LIB}.dylib"
            INTERFACE_INCLUDE_DIRECTORIES
            "${WEBOTS_INCLUDE_PATHS}")
    endif()
endforeach()

#
# webots plugins lib targets
#

if (NOT TARGET webots_plugin_physics)
  add_library(webots_plugin_physics
    ${WEBOTS_HOME}/resources/projects/plugins/physics/physics.c
  )

  target_include_directories(webots_plugin_physics
  PUBLIC
    $<BUILD_INTERFACE:${WEBOTS_HOME}/include>
    $<BUILD_INTERFACE:${WEBOTS_HOME}/include/ode>
  )

  anki_build_target_license(webots_plugin_physics "ANKI")
endif()

list(APPEND WEBOTS_PLUGINS webots_plugin_physics)

#
# Add custom command to set up firewall rules for a given target.
#
macro(webots_setup_target target)
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMENT "Set up webots firewall exception for ${target}"
    COMMAND ${WEBOTS_SETUP} -x $<TARGET_FILE:${target}>
  )
endmacro(webots_setup_target)

#
# Add custom command to set up firewall rules for a given executable.
# Use a timestamp file to determine if exe has changed.
# Add a custom command to create timestamp file,
# then add a custom target that depends on timestamp.
#
macro(webots_setup_exe target exe)
  set(stamp ${exe}.webots_setup_stamp)
  add_custom_command(
    OUTPUT ${stamp}
    DEPENDS ${exe}
    COMMENT "Set up webots firewall exception for ${exe}"
    COMMAND ${WEBOTS_SETUP} -x ${exe}
    COMMAND ${CMAKE_COMMAND} -E touch ${stamp}
  )
  add_custom_target(${target} ALL DEPENDS ${stamp})
endmacro(webots_setup_exe)
