# coretech/common/CMakeLists.txt

project(cti_common)

option(EMBEDDED_BENCHMARK "Enables BeginBenchmark/EndBenchmark functions" OFF)
if(EMBEDDED_BENCHMARK)
  set(additional_defines ANKI_EMBEDDED_BENCHMARK=1)
endif()

if (NOT DEFINED CORETECH)
  set(CORETECH ${CMAKE_CURRENT_SOURCE_DIR}/..)
endif()

include(anki_build_cxx)

anki_build_cxx_library(cti_common ${ANKI_SRCLIST_DIR} STATIC)
anki_build_target_license(cti_common "ANKI" "CC0,${CMAKE_SOURCE_DIR}/licenses/simple-signal.license")

target_compile_definitions(cti_common
  PUBLIC
  ANKICORETECH_USE_OPENCV=1
  ANKICORETECH_USE_MATLAB=0
  ANKICORETECH_EMBEDDED_USE_OPENCV=1
  ANKICORETECH_EMBEDDED_USE_MATLAB=0
  PRIVATE
  CORETECH_ENGINE
  ${additional_defines}
)

target_include_directories(cti_common
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)

#set_target_properties(cti_common PROPERTIES
#  FOLDER "CoreTech"#/Basestation"
#)

target_compile_options(cti_common
  PRIVATE
  -Os
  ${SYMBOL_VISIBILITY_FLAGS}
)

target_link_libraries(cti_common
  PUBLIC
  ${CTI_GENERATED_LIBS}
  util
  jsoncpp
  opencv_interface
  ${OPENCV_LIBS}
  PRIVATE
  z
)

if (VICOS)
  copy_opencv_android_libs()
  add_dependencies(cti_common
    copy_opencv_libs
  )
endif()

#
# CLAD headers must be generated before building library
#
add_dependencies(cti_common cti_common_clad)

#
# cti_common_robot
#

anki_build_cxx_library(cti_common_robot ${ANKI_SRCLIST_DIR} STATIC)
anki_build_target_license(cti_common_robot "ANKI")

target_link_libraries(cti_common_robot
  PUBLIC
  util
  cti_common_clad_cpplite
  PRIVATE
  opencv_interface
)

target_compile_definitions(cti_common_robot
  PUBLIC
  ANKICORETECH_USE_OPENCV=1
  ANKICORETECH_USE_MATLAB=0
  ANKICORETECH_EMBEDDED_USE_OPENCV=1
  ANKICORETECH_EMBEDDED_USE_MATLAB=0
  PRIVATE
  CORETECH_ROBOT
)

target_include_directories(cti_common_robot
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)


if (MACOSX)

  include(gtest)

  #
  # cti_common_test_shared
  #

  anki_build_cxx_executable(cti_common_test_shared ${ANKI_SRCLIST_DIR})
  anki_build_target_license(cti_common_test_shared "ANKI")

  target_compile_options(cti_common_test_shared
    PRIVATE
    -Wno-undef
  )

  target_compile_definitions(cti_common_test_shared
    PRIVATE
    TEST_DATA_PATH=${CORETECH}
  )

  target_link_libraries(cti_common_test_shared
    PRIVATE
    cti_common
    util
    gtest
    ${OPENCV_LIBS}
  )

  #
  # cti_common_test_engine
  #

  anki_build_cxx_executable(cti_common_test_engine ${ANKI_SRCLIST_DIR})
  anki_build_target_license(cti_common_test_engine "ANKI")

  target_compile_definitions(cti_common_test_engine
    PRIVATE
    TEST_DATA_PATH=${CORETECH}
  )

  target_link_libraries(cti_common_test_engine
    PRIVATE
    cti_common
    util
    gtest
    ${OPENCV_LIBS}
  )

  #
  # test harness
  #
  enable_testing()

  add_test(NAME cti_common_test_shared COMMAND cti_common_test_shared)

  set_tests_properties(cti_common_test_shared
    PROPERTIES
    ENVIRONMENT "GTEST_OUTPUT=xml:ctiCommonSharedGoogleTest.xml"
  )

  add_test(NAME cti_common_test_engine COMMAND cti_common_test_engine)

  set_tests_properties(cti_common_test_engine
    PROPERTIES
    ENVIRONMENT "GTEST_OUTPUT=xml:ctiCommonEngineGoogleTest.xml"
  )

endif()
