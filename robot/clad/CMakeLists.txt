#
# clad/robot/CMakeLists.txt
#
# CMake declarations to generate robot CLAD
#
include(generate_clad)

set(CLAD_BASE_DIR "${CMAKE_SOURCE_DIR}/victor-clad/tools/message-buffers")
set(CLAD_EMITTER_DIR "${CLAD_BASE_DIR}/emitters")
set(CLAD_CPPLITE "${CLAD_EMITTER_DIR}/CPP_emitter.py")
set(CLAD_HASH "${CLAD_EMITTER_DIR}/ASTHash_emitter.py")
set(VICTOR_EMITTER_DIR "${CMAKE_SOURCE_DIR}/victor-clad/victorEmitters")

set(ENGINE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/clad/engine")
set(ROBOT_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/clad/robot")
set(CLAD_PY_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/cladPython")

file(STRINGS "${ANKI_SRCLIST_DIR}/robot_clad.srcs.lst" ROBOT_SRCS)

set(CLAD_INCLUDES
    "${CMAKE_SOURCE_DIR}/robot/clad/src"
    "${CMAKE_SOURCE_DIR}/coretech/vision/clad_src"
)

set(ENGINE_CLAD_INCLUDES
    "${CMAKE_SOURCE_DIR}/clad/src/"
    "${CMAKE_SOURCE_DIR}/lib/util/source/anki/clad/"
    "${CMAKE_SOURCE_DIR}/robot/clad/src/"
    "${CMAKE_SOURCE_DIR}/coretech/vision/clad_src/"
    "${CMAKE_SOURCE_DIR}/coretech/common/clad_src/"
)

generate_clad_cpp(
    SRCS ${ROBOT_SRCS}
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${ENGINE_OUTPUT_DIR}"
    INCLUDES "${ENGINE_CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_CPP_GEN_ROBOT_SRCS
)

generate_clad_py(
    TARGET "robot_clad_python"
    SRCS ${ROBOT_SRCS}
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${CLAD_PY_OUTPUT_DIR}"
    INCLUDES "${ENGINE_CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_PY_GEN_ROBOT_SRCS
)

file(STRINGS "${ANKI_SRCLIST_DIR}/robot_clad.headers.lst" ROBOT_HASH_SRCS)

generate_clad(
    SRCS "${ROBOT_HASH_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${ENGINE_OUTPUT_DIR}"
    OUTPUT_FILE true
    EMITTER "${CLAD_HASH}"
    OUTPUT_EXTS "_hash.h"
    INCLUDES "${ENGINE_CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_HASH_GEN_ROBOT_SRCS
)

add_library(robot_clad STATIC
    ${CLAD_CPP_GEN_ROBOT_SRCS}
    ${CLAD_HASH_GEN_ROBOT_SRCS}
)

target_include_directories(robot_clad
    PUBLIC
    $<BUILD_INTERFACE:${ENGINE_OUTPUT_DIR}>
)

target_compile_definitions(robot_clad
  PUBLIC
  USES_CLAD
)

target_link_libraries(robot_clad
    cti_vision_clad
    clad
)

anki_build_target_license(robot_clad "ANKI")

set(CLAD_CPPLITE_NS_FLAG "")

# uncomment to set a global emiiter-namespace
# set(CLAD_CPPLITE_NS_FLAG "--namespace" "CppLite")

set(CLAD_CPPLITE_FLAGS
    --max-message-size 1400
    ${CLAD_CPPLITE_NS_FLAG})

generate_clad_cpplite(
    SRCS ${ROBOT_SRCS}
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${ROBOT_OUTPUT_DIR}"
    FLAGS "${CLAD_CPPLITE_FLAGS}"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_CPP_GEN_ROBOT_SRCS
)

#
# Accumulate sources from running special emitters on union clad files
#
set(CLAD_UNION_EMITTER_SRCS "")

file(STRINGS "${ANKI_SRCLIST_DIR}/robot_clad.headers.lst" ROBOT_UNION_SRCS)

generate_clad(
    SRCS "${ROBOT_UNION_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${ROBOT_OUTPUT_DIR}"
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

set(CLAD_CPP_DECL "${VICTOR_EMITTER_DIR}/cozmo_CPP_declarations_emitter.py")
set(CLAD_CPP_DECL_EXTS "_declarations.def")

generate_clad(
    SRCS "${ROBOT_UNION_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${ROBOT_OUTPUT_DIR}"
    EMITTER ${CLAD_CPP_DECL}
    OUTPUT_EXTS ${CLAD_CPP_DECL_EXTS}
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_DECL_GEN_ROBOT_SRCS
)

list(APPEND CLAD_UNION_EMITTER_SRCS ${CLAD_DECL_GEN_ROBOT_SRCS})

#
# switch emitter
#

set(CLAD_CPP_SWITCH "${VICTOR_EMITTER_DIR}/cozmo_CPPLite_switch_emitter.py")
set(CLAD_CPP_SWITCH_EXTS "_switch.def")

generate_clad(
    SRCS "${ROBOT_UNION_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${ROBOT_OUTPUT_DIR}"
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
set(CLAD_CPP_SWITCH_SUPERVISOR_FLAGS
    --start ${SUPERVISOR_START_TAG}
    --end ${SUPERVISOR_END_TAG}
    ${CLAD_CPPLITE_NS_FLAG})

generate_clad(
    SRCS "${ROBOT_UNION_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${ROBOT_OUTPUT_DIR}"
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
set(CLAD_CPP_SWITCH_ANIM_FLAGS 
    --start ${ANIM_START_TAG}
    --end ${ANIM_END_TAG}
    ${CLAD_CPPLITE_NS_FLAG})

generate_clad(
    SRCS "${ROBOT_UNION_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${ROBOT_OUTPUT_DIR}"
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

set(CLAD_CPP_SEND_HELPER "${VICTOR_EMITTER_DIR}/cozmo_CPPLite_send_helper_emitter.py")
set(CLAD_CPP_SEND_HELPER_EXTS "_send_helper.h")

generate_clad(
    SRCS "${ROBOT_UNION_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${ROBOT_OUTPUT_DIR}"
    EMITTER ${CLAD_CPP_SEND_HELPER}
    OUTPUT_EXTS ${CLAD_CPP_SEND_HELPER_EXTS}
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_SEND_HELPER_GEN_ROBOT_SRCS
)

list(APPEND CLAD_UNION_EMITTER_SRCS ${CLAD_SEND_HELPER_GEN_ROBOT_SRCS})

#
# sendAnimToRobot helper
#

set(CLAD_CPP_A2R_HELPER "${VICTOR_EMITTER_DIR}/cozmo_CPPLite_sendAnimToRobot_helper_emitter.py")
set(CLAD_CPP_A2R_HELPER_EXTS "_sendAnimToRobot_helper.h")

generate_clad(
    SRCS "${ROBOT_UNION_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${ROBOT_OUTPUT_DIR}"
    EMITTER ${CLAD_CPP_A2R_HELPER}
    OUTPUT_EXTS ${CLAD_CPP_A2R_HELPER_EXTS}
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_A2R_HELPER_GEN_ROBOT_SRCS
)

list(APPEND CLAD_UNION_EMITTER_SRCS ${CLAD_A2R_HELPER_GEN_ROBOT_SRCS})

#
# sendAnimToEngine helper
#

set(CLAD_CPP_A2E_HELPER "${VICTOR_EMITTER_DIR}/cozmo_CPPLite_sendAnimToEngine_helper_emitter.py")
set(CLAD_CPP_A2E_HELPER_EXTS "_sendAnimToEngine_helper.h")

generate_clad(
    SRCS "${ROBOT_UNION_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${ROBOT_OUTPUT_DIR}"
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
#   TODO:REG  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/generated/clad>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/generated/clad/robot>
)

target_compile_definitions(robot_clad_cpplite
    PUBLIC
    USES_CPPLITE
)

anki_build_target_license(robot_clad_cpplite "ANKI")

add_dependencies(robot_clad_cpplite cti_vision_clad_cpplite)
