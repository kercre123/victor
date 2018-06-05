
project(utilUnitTest LANGUAGES CXX)

include(anki_build_cxx)

include(gtest)

get_filename_component(util ${CMAKE_CURRENT_SOURCE_DIR}/../../.. ABSOLUTE)

set(UTIL_INCLUDE_DIR ${util})
set(UTIL_LIBRARY util)

set(JSONCPP_INCLUDE_DIR ${util}/source/3rd/jsoncpp)
set(JSONCPP_LIBRARY jsoncpp)

anki_build_cxx_executable(utilUnitTest ${ANKI_SRCLIST_DIR})
anki_build_target_license(utilUnitTest "ANKI")

target_compile_definitions(utilUnitTest PRIVATE
  UNIT_TEST=1
)

target_include_directories(utilUnitTest PRIVATE
  ${UTIL_INCLUDE_DIR}
  ${JSONCPP_INCLUDE_DIR}
)

target_link_libraries(utilUnitTest 
  PRIVATE
  util
  ${JSONCPP_LIBRARY}
  gtest
)

