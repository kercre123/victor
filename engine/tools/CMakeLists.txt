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

if (USE_ENGINEANIM_COMBINED)
  add_library(vic-engine SHARED engined/cozmoEngineMain.cpp)
else ()
  anki_build_cxx_executable(vic-engine ${ANKI_SRCLIST_DIR})
endif()

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

anki_build_strip(TARGET vic-engine)

if (USE_ENGINEANIM_COMBINED)
  anki_build_cxx_executable(vic-engine-anim ${ANKI_SRCLIST_DIR})
  anki_build_target_license(vic-engine-anim "ANKI")

  target_compile_definitions(vic-engine-anim PUBLIC VIC_ENGINE_MAIN=1)

  target_link_libraries(vic-engine-anim
    PRIVATE
    dl
    # platform
    ${PLATFORM_LIBS}
    ${ASAN_EXE_LINKER_FLAGS}
  )

  anki_build_strip(TARGET vic-engine-anim)
endif()
