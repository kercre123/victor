include(anki_build_copy_assets)

anki_build_copy_assets(
    TARGET cozmo_resources_assets
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
)

anki_build_copy_assets(
    TARGET cozmo_resources_sound
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
)

anki_build_copy_assets(
    TARGET cozmo_resources_config
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
)

anki_build_copy_assets(
    TARGET cozmo_resources_webserver
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
)

anki_build_copy_assets(
    TARGET cozmo_bundle
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets"
)

anki_build_copy_assets(
  TARGET        cozmo_resources_tts
  SRCLIST_DIR   ${ANKI_SRCLIST_DIR}
  OUTPUT_DIR   "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
)

set(ASSET_TARGETS
    cozmo_resources_assets
    cozmo_resources_config
    cozmo_resources_webserver
    cozmo_bundle)

# Bundle test assets on OSX
if (MACOSX)
    anki_build_copy_assets(
        TARGET cozmo_resources_test
        SRCLIST_DIR ${ANKI_SRCLIST_DIR}
        OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
    )
    list(APPEND ASSET_TARGETS cozmo_resources_test)
endif(MACOSX)

add_dependencies(cozmo_engine
    ${ASSET_TARGETS}
)

add_dependencies(victor_anim
    ${ASSET_TARGETS}
)