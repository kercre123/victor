cmake_minimum_required(VERSION 3.6)

project(signal_essence_proj)

add_library(signal_essence STATIC)

include(signalessence)

target_link_libraries(signal_essence
  PRIVATE
    ${SIGNALESSENCE_LIBS}
)

target_sources(signal_essence
  PRIVATE
    "${SIGNALESSENCE_PROJ_SRC}/crossoverconfig.c"
    "${SIGNALESSENCE_PROJ_SRC}/fdsearchconfig.c"
    "${SIGNALESSENCE_PROJ_SRC}/mmif_proj.c"
    "${SIGNALESSENCE_PROJ_SRC}/policy_actions.c"
    "${SIGNALESSENCE_PROJ_SRC}/spatialfilterconfig.c"
    "${SIGNALESSENCE_VAD_SRC}/nfbin_f32_anki.c"
    "${SIGNALESSENCE_VAD_SRC}/svad.c"
)

set_target_properties(signal_essence PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES
  "${SIGNALESSENCE_INCLUDE_PATHS}"
)

# anki_build_target_license(signal_essence "ANKI" "Commercial")

set_source_files_properties(${SIGNALESSENCE_PROJ_SRC}/policy_actions.c PROPERTIES COMPILE_FLAGS -Wno-shorten-64-to-32)
