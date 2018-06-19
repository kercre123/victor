#
# platform/dasmgr/CMakeLists.txt
#

project(vic-dasmgr)

include(anki_build_cxx)

if (VICOS)

  anki_build_cxx_executable(vic-dasmgr ${ANKI_SRCLIST_DIR})
  anki_build_target_license(vic-dasmgr "ANKI")

  target_compile_options(vic-dasmgr PRIVATE -Wconversion -Wsign-conversion -Wshorten-64-to-32 -Wsign-compare)

  target_include_directories(vic-dasmgr
    PRIVATE
    ${CMAKE_SOURCE_DIR}
  )

  target_link_libraries(vic-dasmgr
    PRIVATE
    victorCrashReports
    osState
    DAS
    cti_common
    util
  )

  anki_build_strip(TARGET vic-dasmgr)

endif(VICOS)
