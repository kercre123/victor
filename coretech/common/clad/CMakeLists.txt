project(cti_common_clad)

include(generate_clad)

file(STRINGS "${ANKI_SRCLIST_DIR}/cti_common_clad.srcs.lst" SRCS)

set(CLAD_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/"
)

set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../generated/clad/common")

generate_clad_cpp(
    LIBRARY "cti_common_clad"
    SRCS "${SRCS}"
    RELATIVE_SRC_DIR "src"
    OUTPUT_DIR "${OUTPUT_DIR}"
    INCLUDES "${CLAD_INCLUDES}"
    OUTPUT_SRCS CLAD_CPP_GEN_SRCS
)

target_include_directories(cti_common_clad
    PUBLIC
    $<BUILD_INTERFACE:${OUTPUT_DIR}>
)

target_link_libraries(cti_common_clad
    clad
)
