# coretech/vision/CMakeLists.txt

project(cti_vision)

set(CORETECH_VISION_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/anki/vision)

include(okao)

include(anki_build_cxx_library)

# add_library()
anki_build_cxx_library(cti_vision ${ANKI_SRCLIST_DIR} STATIC)

set_target_properties(cti_vision PROPERTIES
  FOLDER "CoreTech"#/Basestation"
)

target_link_libraries(cti_vision
  cti_common
  jsoncpp
  util
  ${OPENCV_LIBS}
  ${OKAO_LIBS}
)

target_compile_options(cti_vision
PRIVATE
  -Os
)

set_property(TARGET cti_vision PROPERTY CXX_STANDARD 11)

target_include_directories(cti_vision
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/basestation/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>  # For access to engine/vision/nearestNeighborLibraryData.h 
  )

target_compile_definitions(cti_vision PRIVATE
  -DCORETECH_BASESTATION
  -DANKICORETECH_USE_OPENCV=1
  -DFACE_TRACKER_OKAO=3
  -DFACE_TRACKER_PROVIDER=3
  -DANKICORETECH_EMBEDDED_USE_OPENCV=1
) 

if(UNIT_TEST)
  add_subdirectory(basestation/test)
endif()

if(MATLAB)
  add_subdirectory(robot/mex)

  add_subdirectory(matlab/mex)
  add_subdirectory(matlab/CameraCapture)

  add_subdirectory(matlab/\@VisionMarkerTrained/private)

  if(EMBEDDED_USE_OPENCV)
    add_subdirectory(tools/decisionTree)
  endif()
endif()
