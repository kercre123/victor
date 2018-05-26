#
# platform/dasmgr/CMakeLists.txt
#

project(vic-dasmgr)

include(anki_build_cxx)

if (VICOS)

  anki_build_cxx_executable(vic-dasmgr ${ANKI_SRCLIST_DIR})

  target_include_directories(vic-dasmgr
    PRIVATE
    ${CMAKE_SOURCE_DIR}
  )

  target_link_libraries(vic-dasmgr
    PRIVATE
    victorCrashReports
    util
  )

  anki_build_strip(TARGET vic-dasmgr)

endif(VICOS)
