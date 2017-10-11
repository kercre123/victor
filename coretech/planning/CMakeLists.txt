#
# coretech/planning/CMakeLists.txt
#

project(cti_planning)

if (NOT DEFINED CORETECH)
  set(CORETECH ${CMAKE_CURRENT_SOURCE_DIR}/..)
endif()

include(anki_build_cxx_library)

#
# cti_planning
#

anki_build_cxx_library(cti_planning ${ANKI_SRCLIST_DIR} STATIC)
  
target_compile_definitions(cti_planning
  PRIVATE
  CORETECH_BASESTATION
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
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if (MACOSX)

  enable_testing()

  include(gtest)

  #
  # cti_planning_test
  #

  anki_build_cxx_executable(cti_planning_test ${ANKI_SRCLIST_DIR})

  target_compile_definitions(cti_planning_test
    PRIVATE
    CORETECH_BASESTATION
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

endif()
