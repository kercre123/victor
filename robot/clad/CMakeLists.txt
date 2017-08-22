include(generate_clad)

set(CLAD_BASE_DIR "${CMAKE_SOURCE_DIR}/tools/message-buffers")
set(CLAD_EMITTER_DIR "${CLAD_BASE_DIR}/emitters")
set(CLAD_CPPLITE "${CLAD_EMITTER_DIR}/CPP_emitter.py")
set(CLAD_HASH "${CLAD_EMITTER_DIR}/ASTHash_emitter.py")

set(CLAD_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/"
)

#CLAD_CPP_DECL=../robot/clad/cozmo_CPP_declarations_emitter.py
#CLAD_CPP_SWITCH=./cozmo_CPP_switch_emitter.py
#CLAD_HASH=$(CLAD_EMITTER_DIR)/ASTHash_emitter.py
set(CLAD_CPPLITE_FLAGS "--max-message-size 1400")

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/clad/robot")

file(STRINGS "${ANKI_SRCLIST_DIR}/robot_clad_embedded.srcs.lst" ROBOT_SRCS)
generate_clad_cpplite(
    SRCS ${ROBOT_SRCS}
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${OUTPUT_DIR}"
    OUTPUT_SRCS CLAD_CPP_GEN_ROBOT_SRCS
)

file(STRINGS "${ANKI_SRCLIST_DIR}/robot_clad_embedded.headers.lst" ROBOT_HASH_SRCS)
generate_clad(
    SRCS "${ROBOT_HASH_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${OUTPUT_DIR}"
    OUTPUT_FILE true
    EMITTER "${CLAD_HASH}"
    OUTPUT_EXTS "_hash.h"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_HASH_GEN_ROBOT_SRCS
)

add_library(robot_clad_cpplite STATIC
    ${CLAD_CPP_GEN_ROBOT_SRCS}
    ${CLAD_HASH_GEN_ROBOT_SRCS}
)

target_include_directories(robot_clad_cpplite
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/robot/generated/clad>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/robot/generated/clad/robot>
)

#target_link_libraries(robot_clad_cpplite
#    clad
#)

