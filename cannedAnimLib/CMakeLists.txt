cmake_minimum_required(VERSION 3.6)

project(canned_anim_lib)

include(anki_build_cxx)

include(flatbuffers)

anki_build_cxx_library(canned_anim_lib ${ANKI_SRCLIST_DIR} STATIC)
anki_build_target_license(canned_anim_lib "ANKI")

target_link_libraries(canned_anim_lib
    PRIVATE
    # cti
    cti_common_robot
    cti_vision
    cti_messaging_robot
    robot_clad_cpplite
    #vendor
    ${FLATBUFFERS_LIBS}
    PUBLIC
    robot_interface # Needs to be public for cozmoConfig.h
    ${ASAN_LINKER_FLAGS}
)
