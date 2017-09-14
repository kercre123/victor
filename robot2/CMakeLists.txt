cmake_minimum_required(VERSION 3.6)
project(robot2)

include(android_strip)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_COMPILE_DEFS "")
set(PLATFORM_FLAGS "")
if (ANDROID)
  set(PLATFORM_LIBS android log)
elseif (MACOSX)
  find_library(FOUNDATION Foundation)
  set(PLATFORM_LIBS
    ${FOUNDATION}
  )
  set(PLATFORM_FLAGS -fobjc-arc)
endif()


# HACK: halstub is a library generated from halStubs.cpp in this source tree.
# However, the actual definition lives in robot/CMakeLists.txt since the supervisor
# library also depends on it.


include(anki_build_cxx_library)                                                                     
anki_build_cxx_library(spine ${ANKI_SRCLIST_DIR} STATIC)

target_link_libraries(spine
PRIVATE
    halstub
    ${PLATFORM_LIBS}
)

target_include_directories(spine
PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/clad/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/syscon>
)


file(STRINGS "${ANKI_SRCLIST_DIR}/robot_supervisor.srcs.lst" ROBOT2_SRCS)
file(STRINGS "${ANKI_SRCLIST_DIR}/robot_supervisor.headers.lst" ROBOT2_HEADERS)

add_executable(robot_supervisor
    ${ROBOT2_SRCS}
    ${ROBOT2_HEADERS}
)
android_strip(TARGET robot_supervisor)

target_link_libraries(robot_supervisor
PRIVATE
    halstub
    supervisor
    robot_transport
    spine
    robot_interface
    robot_clad_cpplite
    cti_common_robot
    cti_messaging
    ${PLATFORM_LIBS}
)

target_compile_definitions(robot_supervisor
PRIVATE
    -DCOZMO_ROBOT
    -DCORETECH_ROBOT
    -DCOZMO_V2
    -D_DEBUG
    #-DUSING_ANDROID_PHONE # Uncomment if deploying on non-Cozmo android device
)

target_include_directories(robot_supervisor
PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/robot2/hal/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/robot/hal/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/generated/robot>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/generated/clad/robot>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/robot2/hal>
)
