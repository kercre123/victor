include(anki_build_copy_assets)

anki_build_copy_assets(
    TARGET platform_config_bin
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}"
)

anki_build_copy_assets(
    TARGET platform_config_etc
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}"
)
