include(generate_clad)

file(STRINGS "${ANKI_SRCLIST_DIR}/engine_clad.srcs.lst" SRCS)

set(CLAD_BASE_DIR "${CMAKE_SOURCE_DIR}/victor-clad/tools/message-buffers")
set(CLAD_EMITTER_DIR "${CLAD_BASE_DIR}/emitters")
set(CLAD_CPP "${CLAD_EMITTER_DIR}/CPP_emitter.py")
set(CLAD_CS "${CLAD_EMMITER_DIR}/CSharp_emitter.py")
set(CLAD_GO "${CLAD_EMITTER_DIR}/Go_emitter.py")
set(CLAD_PY "${CLAD_EMITTER_DIR}/Python_emitter.py")
set(CLAD_HASH "${CLAD_EMITTER_DIR}/ASTHash_emitter.py")

set(CLAD_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/util/source/anki/clad/"
    "${CMAKE_CURRENT_SOURCE_DIR}/../robot/clad/src/"
    "${CMAKE_CURRENT_SOURCE_DIR}/../coretech/vision/clad_src/"
    "${CMAKE_CURRENT_SOURCE_DIR}/../coretech/common/clad_src/"
)

#CLAD_CPP_DECL=../robot/clad/cozmo_CPP_declarations_emitter.py
#CLAD_CPP_SWITCH=./cozmo_CPP_switch_emitter.py
#CLAD_HASH=$(CLAD_EMITTER_DIR)/ASTHash_emitter.py
set(CLAD_CPP_FLAGS "--output-union-helper-constructors --output-json")

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/clad/engine")
set(CLAD_CS_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/cladCSharp")
set(CLAD_GO_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/cladgo/src")
set(CLAD_PY_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/cladPython")
#set(OUTPUT_DIR "../generated/clad/engine")

#generate_clad(
#    LIBRARY "clad_engine"
#    SRCS "${SRCS}"
#    RELATIVE_SRC_DIR "src"
#    OUTPUT_DIR "${OUTPUT_DIR}"
#    EMITTER "${CLAD_CPP}"
#    OUTPUT_EXTS ".h" ".cpp"
#    FLAGS "${CLAD_CPP_FLAGS}"
#    INCLUDES "${CLAD_INCLUDES}"
#    OUTPUT_SRCS CLAD_GEN_SRCS
#)

file(STRINGS "${ANKI_SRCLIST_DIR}/robot_clad.srcs.lst" ROBOT_SRCS)
generate_clad_cpp(
    SRCS ${ROBOT_SRCS}
    RELATIVE_SRC_DIR "robot/src"
    OUTPUT_DIR "${OUTPUT_DIR}"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_CPP_GEN_ROBOT_SRCS
)

generate_clad_py(
    TARGET "robot_clad_python"
    SRCS ${ROBOT_SRCS}
    RELATIVE_SRC_DIR "robot/src"
    OUTPUT_DIR "${CLAD_PY_OUTPUT_DIR}"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_PY_GEN_ROBOT_SRCS
)

generate_clad_cs(
    TARGET "robot_clad_csharp"
    SRCS ${ROBOT_SRCS}
    RELATIVE_SRC_DIR "robot/src"
    OUTPUT_DIR "${CLAD_CS_OUTPUT_DIR}"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_CS_GEN_ROBOT_SRCS
)

file(STRINGS "${ANKI_SRCLIST_DIR}/robot_clad.headers.lst" ROBOT_HASH_SRCS)
generate_clad(
    SRCS "${ROBOT_HASH_SRCS}"
    RELATIVE_SRC_DIR "robot/src"
    OUTPUT_DIR "${OUTPUT_DIR}"
    OUTPUT_FILE true
    EMITTER "${CLAD_HASH}"
    OUTPUT_EXTS "_hash.h"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_HASH_GEN_ROBOT_SRCS
)

add_library(robot_clad STATIC
    ${CLAD_CPP_GEN_ROBOT_SRCS}
    ${CLAD_HASH_GEN_ROBOT_SRCS}
)

target_include_directories(robot_clad
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/generated/clad/engine>
)

target_link_libraries(robot_clad
    cti_vision_clad
    clad
)

anki_build_target_license(robot_clad "ANKI")

file(STRINGS "${ANKI_SRCLIST_DIR}/viz_clad.srcs.lst" ROBOT_SRCS)
generate_clad_cpp(
    SRCS ${ROBOT_SRCS}
    RELATIVE_SRC_DIR "vizSrc"
    OUTPUT_DIR "${OUTPUT_DIR}"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_CPP_GEN_VIZ_SRCS
)

generate_clad_cpp(
    SRCS "${SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${OUTPUT_DIR}"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_CPP_GEN_SRCS
)

generate_clad_py(
    TARGET "engine_viz_clad_python"
    SRCS ${ROBOT_SRCS}
    RELATIVE_SRC_DIR "vizSrc"
    OUTPUT_DIR "${CLAD_PY_OUTPUT_DIR}"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_PY_GEN_VIZ_SRCS
)

generate_clad_py(
    TARGET "engine_clad_python"
    SRCS "${SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${CLAD_PY_OUTPUT_DIR}"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_PY_GEN_SRCS
)

generate_clad_cs(
    TARGET "engine_viz_clad_csharp"
    SRCS ${ROBOT_SRCS}
    RELATIVE_SRC_DIR "vizSrc"
    OUTPUT_DIR "${CLAD_CS_OUTPUT_DIR}"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_CS_GEN_VIZ_SRCS
)

generate_clad_cs(
    TARGET "engine_clad_csharp"
    SRCS "${SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${CLAD_CS_OUTPUT_DIR}"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_CS_GEN_SRCS
)

file(STRINGS "${ANKI_SRCLIST_DIR}/engine_clad.headers.lst" ENGINE_HASH_SRCS)
generate_clad(
    SRCS "${ENGINE_HASH_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${OUTPUT_DIR}"
    OUTPUT_FILE true
    EMITTER "${CLAD_HASH}"
    OUTPUT_EXTS "_hash.h"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_HASH_GEN_SRCS
)
generate_clad(
    SRCS "${ENGINE_HASH_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${CLAD_PY_OUTPUT_DIR}"
    OUTPUT_FILE true
    EMITTER "${CLAD_HASH}"
    OUTPUT_EXTS "_hash.py"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_HASH_PY_GEN_SRCS
)
generate_clad(
    SRCS "${ENGINE_HASH_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${CLAD_CS_OUTPUT_DIR}"
    OUTPUT_FILE true
    EMITTER "${CLAD_HASH}"
    OUTPUT_EXTS "_hash.cs"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_HASH_CS_GEN_SRCS
)

file(STRINGS "${ANKI_SRCLIST_DIR}/cloud_clad.srcs.lst" CLOUD_SRCS)
generate_clad(
    TARGET "cloud_clad_go"
    SRCS "${CLOUD_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${CLAD_GO_OUTPUT_DIR}"
    EMITTER "${CLAD_GO}"
    OUTPUT_EXTS ".go"
    INCLUDES ""
    OUTPUT_SRCS CLAD_GO_GEN_SRCS
)

set(CLAD_GO_GEN_SRCS ${CLAD_GO_GEN_SRCS} PARENT_SCOPE)
string(REPLACE "cladgo/src" "clad/engine" CLAD_CLOUD_CPP_GEN_SRCS "${CLAD_GO_GEN_SRCS}")
string(REPLACE ".go" ".cpp" CLAD_CLOUD_CPP_GEN_SRCS "${CLAD_CLOUD_CPP_GEN_SRCS}")

file(STRINGS "${ANKI_SRCLIST_DIR}/gateway_clad.srcs.lst" GATEWAY_SRCS)
generate_clad(
    TARGET "gateway_clad_go"
    SRCS "${GATEWAY_SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${CLAD_GO_OUTPUT_DIR}"
    EMITTER "${CLAD_GO}"
    OUTPUT_EXTS ".go"
    INCLUDES ""
    OUTPUT_SRCS CLAD_GATEWAY_GEN_SRCS
)

set(CLAD_GATEWAY_GEN_SRCS ${CLAD_GATEWAY_GEN_SRCS} PARENT_SCOPE)
string(REPLACE "cladgo/src" "clad/engine" CLAD_GATEWAY_CPP_GEN_SRCS "${CLAD_GATEWAY_GEN_SRCS}")
string(REPLACE ".go" ".cpp" CLAD_GATEWAY_CPP_GEN_SRCS "${CLAD_GATEWAY_CPP_GEN_SRCS}")

add_library(cloud_clad_cpp STATIC
    ${CLAD_CLOUD_CPP_GEN_SRCS}
    ${CLAD_GATEWAY_CPP_GEN_SRCS}
)
target_include_directories(cloud_clad_cpp
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/generated/clad/engine>
)
target_link_libraries(cloud_clad_cpp
    clad
)

anki_build_target_license(cloud_clad_cpp "ANKI")

add_custom_target(engine_clad_python_hash ALL DEPENDS ${CLAD_HASH_PY_GEN_SRCS})
add_custom_target(engine_clad_csharp_hash ALL DEPENDS ${CLAD_HASH_CS_GEN_SRCS})

add_library(engine_clad STATIC
    ${CLAD_CPP_GEN_SRCS}
    ${CLAD_HASH_GEN_SRCS}
    ${CLAD_CPP_GEN_VIZ_SRCS}
)

target_include_directories(engine_clad
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/generated/clad/engine>
)

target_link_libraries(engine_clad
    clad
    robot_clad
    sdk_clad_cpp
    cti_common_clad
    cti_vision_clad
    util_clad
)

anki_build_target_license(engine_clad "ANKI")
