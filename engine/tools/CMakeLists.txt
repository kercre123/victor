cmake_minimum_required(VERSION 3.6)

project(engine_tools)

include(anki_build_cxx)
include(anki_build_strip)

set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_FLAGS "")

if (VICOS)
  set(PLATFORM_LIBS log)
elseif (MACOSX)
  find_library(FOUNDATION Foundation)
  set(PLATFORM_LIBS
    ${FOUNDATION}
  )
endif()

anki_build_cxx_executable(vic-engine ${ANKI_SRCLIST_DIR})

if (USE_ENGINEANIM_COMBINED)
  target_compile_definitions(vic-engine PUBLIC USE_ENGINEANIM_COMBINED=1)

  target_link_libraries(vic-engine
    PRIVATE
    # anki libs
    util
    cozmo_engine
    victor_anim
    osState
    cti_common
    victorCrashReports
    # platform
    jsoncpp
    ${PLATFORM_LIBS}
    ${ASAN_EXE_LINKER_FLAGS}
    ${ANKITRACE}
  )
else ()

  target_link_libraries(vic-engine
    PRIVATE
    # anki libs
    util
    cozmo_engine
    osState
    victorCrashReports
    # platform
    ${PLATFORM_LIBS}
    ${ASAN_EXE_LINKER_FLAGS}
  )
endif()

anki_build_strip(TARGET vic-engine)
