include(anki_build_copy_assets)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
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
else()
    anki_build_copy_assets_py(
        TARGET platform_config_bin
        SRCLIST_DIR ${ANKI_SRCLIST_DIR}
        OUTPUT_DIR "${CMAKE_BINARY_DIR}"
    )

    anki_build_copy_assets_py(
        TARGET platform_config_etc
        SRCLIST_DIR ${ANKI_SRCLIST_DIR}
        OUTPUT_DIR "${CMAKE_BINARY_DIR}"
    )
endif()
