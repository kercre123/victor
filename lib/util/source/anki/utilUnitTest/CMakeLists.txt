
project(utilUnitTest LANGUAGES CXX)

include(anki_build_cxx_library)

get_filename_component(util ${CMAKE_CURRENT_SOURCE_DIR}/../../.. ABSOLUTE)

set(UTIL_INCLUDE_DIR ${util})
set(UTIL_LIBRARY util)

set(JSONCPP_INCLUDE_DIR ${util}/source/3rd/jsoncpp)
set(JSONCPP_LIBRARY jsoncpp)

set(GTEST_INCLUDE_DIR ${util}/libs/packaged/include)
set(GTEST_LIBRARY ${util}/libs/framework/gtest.framework)

anki_build_cxx_executable(utilUnitTest ${ANKI_SRCLIST_DIR})

target_compile_definitions(utilUnitTest PRIVATE
  -DUNIT_TEST=1
)

target_include_directories(utilUnitTest PRIVATE
  ${UTIL_INCLUDE_DIR}
  ${JSONCPP_INCLUDE_DIR}
  ${GTEST_INCLUDE_DIR}
)

target_link_libraries(utilUnitTest PRIVATE
  ${UTIL_LIBRARY}
  ${JSONCPP_LIBRARY}
  ${GTEST_LIBRARY}
)

