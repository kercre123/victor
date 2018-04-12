#
# clad/robot/CMakeLists.txt
#
# CMake declarations to generate robot CLAD
#
include(generate_clad)

set(CLAD_BASE_DIR "${CMAKE_SOURCE_DIR}/tools/message-buffers")
set(CLAD_EMITTER_DIR "${CLAD_BASE_DIR}/emitters")
set(CLAD_CPPLITE "${CLAD_EMITTER_DIR}/CPP_emitter.py")
set(CLAD_HASH "${CLAD_EMITTER_DIR}/ASTHash_emitter.py")

set(CLAD_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/"
    "${CMAKE_CURRENT_SOURCE_DIR}/coretech_common_clad/"
    "${CMAKE_CURRENT_SOURCE_DIR}/coretech_vision_clad/"
)

# Path to helper scripts
set(ROBOT_CLAD ${CMAKE_SOURCE_DIR}/robot/clad)

#CLAD_CPP_DECL=../robot/clad/cozmo_CPP_declarations_emitter.py
#CLAD_CPP_SWITCH=./cozmo_CPP_switch_emitter.py
#CLAD_HASH=$(CLAD_EMITTER_DIR)/ASTHash_emitter.py
set(CLAD_CPPLITE_FLAGS "--max-message-size 1400")

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/clad/robot")
# set(PY_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/cladPython")

file(STRINGS "${ANKI_SRCLIST_DIR}/robot_clad_embedded.srcs.lst" ROBOT_SRCS)

generate_clad_cpplite(
    SRCS ${ROBOT_SRCS}
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${OUTPUT_DIR}"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_CPP_GEN_ROBOT_SRCS
)

# generate_clad_py(
#     SRCS ${ROBOT_SRCS}
#     RELATIVE_SRC_DIR "src"
#     OUTPUT_DIR "${PY_OUTPUT_DIR}"
#     OUTPUT_SRCS CLAD_PY_GEN_ROBOT_SRCS
# )

#
# Accumulate sources from running special emitters on union clad files
#
set(CLAD_UNION_EMITTER_SRCS "")

file(STRINGS "${ANKI_SRCLIST_DIR}/robot_clad_embedded.headers.lst" ROBOT_UNION_SRCS)

generate_clad(
    SRCS "${ROBOT_UNION_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${OUTPUT_DIR}"
    OUTPUT_FILE true
    EMITTER "${CLAD_HASH}"
    OUTPUT_EXTS "_hash.h"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_HASH_GEN_ROBOT_SRCS
)

list(APPEND CLAD_UNION_EMITTER_SRCS ${CLAD_HASH_GEN_ROBOT_SRCS})

#
# declarations emitter
#

set(CLAD_CPP_DECL "${ROBOT_CLAD}/cozmo_CPP_declarations_emitter.py")
set(CLAD_CPP_DECL_EXTS "_declarations.def")

generate_clad(
    SRCS "${ROBOT_UNION_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${OUTPUT_DIR}"
    EMITTER ${CLAD_CPP_DECL}
    OUTPUT_EXTS ${CLAD_CPP_DECL_EXTS}
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_DECL_GEN_ROBOT_SRCS
)

list(APPEND CLAD_UNION_EMITTER_SRCS ${CLAD_DECL_GEN_ROBOT_SRCS})

#
# switch emitter
#

set(CLAD_CPP_SWITCH "${ROBOT_CLAD}/cozmo_CPPLite_switch_emitter.py")
set(CLAD_CPP_SWITCH_EXTS "_switch.def")

generate_clad(
    SRCS "${ROBOT_UNION_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${OUTPUT_DIR}"
    EMITTER ${CLAD_CPP_SWITCH}
    OUTPUT_EXTS ${CLAD_CPP_SWITCH_EXTS}
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_SWITCH_GEN_ROBOT_SRCS
)

list(APPEND CLAD_UNION_EMITTER_SRCS ${CLAD_SWITCH_GEN_ROBOT_SRCS})

#
# supervisor switch
#

set(SUPERVISOR_START_TAG "0x01")
set(SUPERVISOR_END_TAG "0x4F")
set(CLAD_CPP_SWITCH_SUPERVISOR_EXTS "_switch_from_${SUPERVISOR_START_TAG}_to_${SUPERVISOR_END_TAG}.def")
set(CLAD_CPP_SWITCH_SUPERVISOR_FLAGS "--start=${SUPERVISOR_START_TAG}" "--end=${SUPERVISOR_END_TAG}")

generate_clad(
    SRCS "${ROBOT_UNION_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${OUTPUT_DIR}"
    EMITTER ${CLAD_CPP_SWITCH}
    OUTPUT_EXTS ${CLAD_CPP_SWITCH_SUPERVISOR_EXTS}
    FLAGS ${CLAD_CPP_SWITCH_SUPERVISOR_FLAGS}
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_SUPERVISOR_GEN_ROBOT_SRCS
)

list(APPEND CLAD_UNION_EMITTER_SRCS ${CLAD_SUPERVISOR_GEN_ROBOT_SRCS})

#
# animation switch
#

set(ANIM_START_TAG "0x50")
set(ANIM_END_TAG "0xAF")
set(CLAD_CPP_SWITCH_ANIM_EXTS "_switch_from_${ANIM_START_TAG}_to_${ANIM_END_TAG}.def")
set(CLAD_CPP_SWITCH_ANIM_FLAGS "--start=${ANIM_START_TAG}" "--end=${ANIM_END_TAG}")

generate_clad(
    SRCS "${ROBOT_UNION_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${OUTPUT_DIR}"
    EMITTER ${CLAD_CPP_SWITCH}
    OUTPUT_EXTS ${CLAD_CPP_SWITCH_ANIM_EXTS}
    FLAGS ${CLAD_CPP_SWITCH_ANIM_FLAGS}
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_SUPERVISOR_GEN_ANIM_SRCS
)

list(APPEND CLAD_UNION_EMITTER_SRCS ${CLAD_SUPERVISOR_GEN_ANIM_SRCS})

#
# send helper
#

set(CLAD_CPP_SEND_HELPER "${ROBOT_CLAD}/cozmo_CPPLite_send_helper_emitter.py")
set(CLAD_CPP_SEND_HELPER_EXTS "_send_helper.h")

generate_clad(
    SRCS "${ROBOT_UNION_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${OUTPUT_DIR}"
    EMITTER ${CLAD_CPP_SEND_HELPER}
    OUTPUT_EXTS ${CLAD_CPP_SEND_HELPER_EXTS}
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_SEND_HELPER_GEN_ROBOT_SRCS
)

list(APPEND CLAD_UNION_EMITTER_SRCS ${CLAD_SEND_HELPER_GEN_ROBOT_SRCS})

#
# sendAnimToRobot helper
#

set(CLAD_CPP_A2R_HELPER "${ROBOT_CLAD}/cozmo_CPPLite_sendAnimToRobot_helper_emitter.py")
set(CLAD_CPP_A2R_HELPER_EXTS "_sendAnimToRobot_helper.h")

generate_clad(
    SRCS "${ROBOT_UNION_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${OUTPUT_DIR}"
    EMITTER ${CLAD_CPP_A2R_HELPER}
    OUTPUT_EXTS ${CLAD_CPP_A2R_HELPER_EXTS}
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_A2R_HELPER_GEN_ROBOT_SRCS
)

list(APPEND CLAD_UNION_EMITTER_SRCS ${CLAD_A2R_HELPER_GEN_ROBOT_SRCS})

#
# sendAnimToEngine helper
#

set(CLAD_CPP_A2E_HELPER "${ROBOT_CLAD}/cozmo_CPPLite_sendAnimToEngine_helper_emitter.py")
set(CLAD_CPP_A2E_HELPER_EXTS "_sendAnimToEngine_helper.h")

generate_clad(
    SRCS "${ROBOT_UNION_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${OUTPUT_DIR}"
    EMITTER ${CLAD_CPP_A2E_HELPER}
    OUTPUT_EXTS ${CLAD_CPP_A2E_HELPER_EXTS}
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_A2E_HELPER_GEN_ROBOT_SRCS
)

list(APPEND CLAD_UNION_EMITTER_SRCS ${CLAD_A2E_HELPER_GEN_ROBOT_SRCS})

#
# robot_clad_cpplite
#

add_library(robot_clad_cpplite STATIC
    ${CLAD_CPP_GEN_ROBOT_SRCS}
    # ${CLAD_PY_GEN_ROBOT_SRCS}
    ${CLAD_UNION_EMITTER_SRCS}
)

target_include_directories(robot_clad_cpplite
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/generated/clad>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/generated/clad/robot>
)

add_dependencies(robot_clad_cpplite cti_vision_clad_cpplite)

