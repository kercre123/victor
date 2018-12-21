# coretech/messaging/CMakeLists.txt

project(cti_messaging)

include(anki_build_cxx)

anki_build_cxx_library(cti_messaging ${ANKI_SRCLIST_DIR} STATIC)
anki_build_target_license(cti_messaging "ANKI")

set_target_properties(cti_messaging PROPERTIES
  FOLDER "CoreTech"#/Basestation"
)

target_compile_definitions(cti_messaging
  PRIVATE
  CORETECH_ENGINE
)

target_link_libraries(cti_messaging
  PRIVATE
  cti_common
  clad
  ${ASAN_LINKER_FLAGS}
)

target_include_directories(cti_messaging
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)

anki_build_cxx_library(cti_messaging_robot ${ANKI_SRCLIST_DIR} STATIC)
anki_build_target_license(cti_messaging_robot "ANKI")

target_compile_definitions(cti_messaging_robot
  PRIVATE
  CORETECH_ROBOT
)

target_include_directories(cti_messaging_robot
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)

target_link_libraries(cti_messaging_robot
  PUBLIC
  cti_common_robot
  ${ASAN_LINKER_FLAGS}
)

anki_build_cxx_library(cti_messaging_shared ${ANKI_SRCLIST_DIR} STATIC)
anki_build_target_license(cti_messaging_shared "ANKI")

target_compile_definitions(cti_messaging_shared
  PRIVATE
  CORETECH_SHARED
)

target_include_directories(cti_messaging_shared
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/lib/util/source/anki>
)

target_link_libraries(cti_messaging_shared
  PUBLIC
  cti_common_shared
  ${ASAN_LINKER_FLAGS}
)
