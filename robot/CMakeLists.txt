
project(robot)

add_library(robot_interface INTERFACE IMPORTED GLOBAL)
set_property(TARGET robot_interface
             PROPERTY INTERFACE_INCLUDE_DIRECTORIES
               ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(clad)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
set(PLATFORM_FLAGS "")
if (MACOSX)
  find_library(FOUNDATION Foundation)
  set(PLATFORM_COMPILE_DEFS
    SIMULATOR
  )
elseif (ANDROID)
  set(PLATFORM_COMPILE_DEFS
    DEVELOPMENT
  )
endif()

#
# HACK #1
#
# robot_hal is defined here, because supervisor depends on it, even though
# the source file lives in robot2. This will be cleaned up after we fork
# the victor repo
#
add_library(robot_hal STATIC
  ${CMAKE_SOURCE_DIR}/robot2/hal/src/hal.cpp
  ${CMAKE_SOURCE_DIR}/robot2/hal/src/hal_motors.cpp
)

target_link_libraries(robot_hal
  PRIVATE
  cti_common_robot
  robot_interface
  robot_clad_cpplite
)

target_include_directories(robot_hal
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/robot2/syscon>
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/robot2/hal/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/robot2/hal/sim/include>
)

target_compile_definitions(robot_hal
  PRIVATE
  COZMO_ROBOT
  ${PLATFORM_COMPILE_DEFS}
)

# HACK #2 - supervisor build on mac requires files in 
# robot2/ and simulator/ folders. At least the ones in robot2 (sim_hal)
# also depend on files in supervisor :-(
# Rather than break the circular dependency now, we just 
# include those sources when building for mac

set(PLATFORM_SRCS "")
set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
set(PLATFORM_FLAGS "")
if (ANDROID)
  set(PLATFORM_LIBS
    robot_hal
  )
  set(PLATFORM_COMPILE_DEFS
    DEVELOPMENT
  )
elseif (MACOSX)
  include(webots)

  set(PLATFORM_LIBS
      util
      cti_messaging
      ${WEBOTS_LIBS}
  )
  set(PLATFORM_INCLUDES
      # FIXME gross: ("simulator/robot/sim_overlayDisplay.h")
      ${CMAKE_SOURCE_DIR}
      ${CMAKE_SOURCE_DIR}/robot2/hal/include
      ${CMAKE_SOURCE_DIR}/robot2/hal/sim/include
  )
  set(PLATFORM_COMPILE_DEFS
      SIMULATOR
  )
  set(PLATFORM_SRCS
      ${CMAKE_SOURCE_DIR}/robot2/hal/sim/src/sim_hal.cpp
      ${CMAKE_SOURCE_DIR}/robot2/hal/sim/src/sim_radio.cpp
      ${CMAKE_SOURCE_DIR}/simulator/robot/sim_overlayDisplay.cpp
  )
endif()


include(anki_build_cxx)

anki_build_cxx_library(robot_transport ${ANKI_SRCLIST_DIR} STATIC)

target_compile_definitions(robot_transport
  PRIVATE
  COZMO_ROBOT
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(robot_transport
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)


anki_build_cxx_library(supervisor ${ANKI_SRCLIST_DIR} STATIC
  ${PLATFORM_SRCS}
)

target_link_libraries(supervisor
  PRIVATE
  robot_interface
  robot_clad_cpplite
  cti_common_robot
  cti_planning_robot
  ${PLATFORM_LIBS}
)

target_compile_definitions(supervisor
  PRIVATE
  COZMO_ROBOT
  CORETECH_ROBOT
  _DEBUG
  ${PLATFORM_COMPILE_DEFS}
  #-DUSING_ANDROID_PHONE # Uncomment if deploying on non-Cozmo android device
)

target_include_directories(supervisor
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/supervisor/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/generated/clad>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/generated/clad/robot>
  PUBLIC
  ${PLATFORM_INCLUDES}
)

