# coretech/planning/CMakeLists.txt

project(cti_planning)

include(anki_build_cxx_library)
anki_build_cxx_library(cti_planning ${ANKI_SRCLIST_DIR} STATIC)
  
target_compile_definitions(cti_planning
  PRIVATE
  -DCORETECH_BASESTATION
  -DANKICORETECH_USE_OPENCV=1
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

target_include_directories(cti_planning PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
