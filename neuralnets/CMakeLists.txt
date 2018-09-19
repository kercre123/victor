cmake_minimum_required(VERSION 3.6)

if(VICOS)

  project(vic-neuralnets)

  include(opencv)
  include(anki_build_strip)
  include(anki_build_cxx)

  #
  # Executable: vic-neuralnets process 
  #
  #   (only on robot; see webotsCtrlNeuralNets for analogous process on mac/sim)
  #

  anki_build_cxx_executable(vic-neuralnets ${ANKI_SRCLIST_DIR})
  anki_build_target_license(vic-neuralnets "ANKI")

  target_link_libraries(vic-neuralnets
    PRIVATE
    jsoncpp
    util
    cti_common
    cti_neuralnets
    cti_vision
    ${OPENCV_LIBS}
  )

  anki_build_strip(TARGET vic-neuralnets)

endif()