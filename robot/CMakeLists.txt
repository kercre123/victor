
cmake_minimum_required(VERSION 3.6)

project(robot)

include(anki_build_cxx)
include(anki_build_strip)

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
endif()


add_library(robot_hal STATIC
  ${CMAKE_SOURCE_DIR}/robot/hal/src/hal.cpp
  ${CMAKE_SOURCE_DIR}/robot/hal/src/hal_motors.cpp
)

target_link_libraries(robot_hal
  PRIVATE
  cti_common_robot
  robot_interface
  robot_clad_cpplite
  hdr_histogram
)

target_include_directories(robot_hal
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/robot/syscon>
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/robot/hal/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/robot/hal/sim/include>
)

target_compile_options(robot_hal
  PRIVATE
  ${ANKI_BUILD_CXX_COMPILE_OPTIONS}
)

target_compile_definitions(robot_hal
  PRIVATE
  COZMO_ROBOT
  ${PLATFORM_COMPILE_DEFS}
  ${ANKI_BUILD_CXX_COMPILE_DEFINITIONS}
  #HAL_DUMMY_BODY # Uncomment if deploying on head only
)

anki_build_target_license(robot_hal "ANKI")

# Create a lib for cube firmware led animation code, so
# we can link into it from webotsCtrlLightCube
if (MACOSX)
add_library(cube_led_animator STATIC
  ${CMAKE_SOURCE_DIR}/robot/cube_firmware/app/animation.c
)

target_compile_options(cube_led_animator
  PRIVATE
  ${ANKI_BUILD_CXX_COMPILE_OPTIONS}
)

target_compile_definitions(cube_led_animator
  PRIVATE
  ${PLATFORM_COMPILE_DEFS}
  ${ANKI_BUILD_CXX_COMPILE_DEFINITIONS}
)
endif()

set(PLATFORM_SRCS "")
set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
set(PLATFORM_FLAGS "")
if (VICOS)
  set(PLATFORM_LIBS
    robot_hal
  )
elseif (MACOSX)
  include(webots)

  set(PLATFORM_LIBS
      util
      util_audio
      cti_messaging
      ${WEBOTS_LIBS}
  )
  set(PLATFORM_INCLUDES
      # FIXME gross: ("simulator/robot/sim_overlayDisplay.h")
      ${CMAKE_SOURCE_DIR}
      ${CMAKE_SOURCE_DIR}/robot/hal/include
      ${CMAKE_SOURCE_DIR}/robot/hal/sim/include
  )
  set(PLATFORM_COMPILE_DEFS
      SIMULATOR
  )
  set(PLATFORM_SRCS
      ${CMAKE_SOURCE_DIR}/robot/hal/sim/src/sim_hal.cpp
      ${CMAKE_SOURCE_DIR}/robot/hal/src/radio.cpp
      ${CMAKE_SOURCE_DIR}/simulator/robot/sim_overlayDisplay.cpp
  )
endif()

anki_build_cxx_library(robot_transport ${ANKI_SRCLIST_DIR} STATIC)
anki_build_target_license(robot_transport "ANKI")

target_compile_definitions(robot_transport
  PRIVATE
  COZMO_ROBOT
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(robot_transport
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

add_executable(displayFaultCode
  ${CMAKE_SOURCE_DIR}/robot/include/anki/cozmo/shared/factory/faultCodeMain.cpp
)
target_compile_options(displayFaultCode PRIVATE ${ANKI_BUILD_CXX_COMPILE_OPTIONS})
target_compile_definitions(displayFaultCode PRIVATE ${ANKI_BUILD_CXX_COMPILE_DEFINITIONS})
anki_build_target_license(displayFaultCode "ANKI")
anki_build_cxx_library(supervisor ${ANKI_SRCLIST_DIR} STATIC
  ${PLATFORM_SRCS}
)
anki_build_target_license(supervisor "ANKI")

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
  ${PLATFORM_COMPILE_DEFS}
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

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
set(PLATFORM_FLAGS "")
if (VICOS)
  set(PLATFORM_LIBS log)
elseif (MACOSX)
  find_library(FOUNDATION Foundation)
  set(PLATFORM_LIBS
    ${FOUNDATION}
  )
  set(PLATFORM_COMPILE_DEFS
    SIMULATOR
  )
endif()

#
# spine
#
if (VICOS)

  anki_build_cxx_library(spine ${ANKI_SRCLIST_DIR} STATIC)
  anki_build_target_license(spine "ANKI")

  target_include_directories(spine
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/clad/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/syscon>
  )

  target_link_libraries(spine
    PRIVATE
    robot_hal
    ${PLATFORM_LIBS}
  )

endif()

#
# robot_supervisor
#
if (VICOS)

  anki_build_cxx_executable(vic-robot ${ANKI_SRCLIST_DIR})
  anki_build_target_license(vic-robot "ANKI")

  target_compile_definitions(vic-robot
    PRIVATE
    COZMO_ROBOT
    CORETECH_ROBOT
    ${PLATFORM_COMPILE_DEFS}
    #HAL_DUMMY_BODY # Uncomment if deploying on head only
  )

  target_include_directories(vic-robot
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/robot/hal>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/robot/hal/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/robot/syscon>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/generated/clad/robot>
  )

  target_link_libraries(vic-robot
    PRIVATE
    robot_hal
    supervisor
    robot_transport
    spine
    robot_interface
    robot_clad_cpplite
    cti_common_robot
    cti_messaging_robot
    victorCrashReports
    ${PLATFORM_LIBS}
  )

  anki_build_strip(TARGET vic-robot)

endif()
