#
# coretech/planning/CMakeLists.txt
#

project(cti_planning)

if (NOT DEFINED CORETECH)
  set(CORETECH ${CMAKE_CURRENT_SOURCE_DIR}/..)
endif()

include(anki_build_cxx)

#
# cti_planning
#

anki_build_cxx_library(cti_planning ${ANKI_SRCLIST_DIR} STATIC)
anki_build_target_license(cti_planning "ANKI")

target_compile_definitions(cti_planning
  PRIVATE
  CORETECH_ENGINE
)

set_target_properties(cti_planning PROPERTIES
  FOLDER "CoreTech"#/Basestation"
)

target_compile_options(cti_planning
  PRIVATE
  -stdlib=libc++
  -Os
)

set_property(TARGET cti_planning PROPERTY CXX_STANDARD 11)

target_link_libraries(cti_planning
  PUBLIC
  cti_common
  PRIVATE
  ${OPENCV_LIBS}
)

target_include_directories(cti_planning
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)

#
# cti_planning_robot
#

anki_build_cxx_library(cti_planning_robot ${ANKI_SRCLIST_DIR} STATIC)
anki_build_target_license(cti_planning_robot "ANKI")

target_compile_definitions(cti_planning_robot
  PRIVATE
  CORETECH_ROBOT
)

set_target_properties(cti_planning_robot PROPERTIES
  FOLDER "CoreTech"#/Basestation"
)

target_compile_options(cti_planning_robot
  PRIVATE
  -stdlib=libc++
  -Os
)

set_property(TARGET cti_planning_robot PROPERTY CXX_STANDARD 11)

target_link_libraries(cti_planning_robot
  PUBLIC
  cti_common_robot
)

target_include_directories(cti_planning_robot
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)

if (MACOSX)

  #
  # cti_planning_standalone
  #
  
  anki_build_cxx_executable(cti_planning_standalone ${ANKI_SRCLIST_DIR})
  anki_build_target_license(cti_planning_standalone "ANKI")
  target_compile_definitions(cti_planning_standalone
    PRIVATE
    CORETECH_ENGINE
  )

  target_link_libraries(cti_planning_standalone
    PRIVATE
    cti_planning
    ${OPENCV_LIBS}
  )


  enable_testing()

  include(gtest)

  #
  # cti_planning_test
  #

  anki_build_cxx_executable(cti_planning_test ${ANKI_SRCLIST_DIR})
  anki_build_target_license(cti_planning_test "ANKI")

  target_compile_definitions(cti_planning_test
    PRIVATE
    CORETECH_ENGINE
    TEST_DATA_PATH=${CORETECH}
  )

  target_include_directories(cti_planning_test
    PRIVATE
    basestation/src
  )

  target_link_libraries(cti_planning_test
    PRIVATE
    cti_planning
    gtest
    ${OPENCV_LIBS}
  )


  add_test(NAME cti_planning_test COMMAND cti_planning_test)

  set_tests_properties(cti_planning_test
    PROPERTIES
    ENVIRONMENT "GTEST_OUTPUT=xml:ctiPlanningGoogleTest.xml"
  )

endif()
