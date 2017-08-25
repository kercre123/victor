include(anki_build_copy_assets)

anki_build_copy_assets(
    TARGET cozmo_resources_assets
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/assets/cozmo_resources"
)

anki_build_copy_assets(
    TARGET cozmo_resources_config
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/assets/cozmo_resources"
)

anki_build_copy_assets(
    TARGET cozmo_bundle
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/assets"
)

set(ASSET_TARGETS
    cozmo_resources_assets
    cozmo_resources_config
    cozmo_bundle)

set(RESOURCE_LIST_FILE "${CMAKE_BINARY_DIR}/assets/resources.txt")
set(LIST_DIR_EXE ${CMAKE_SOURCE_DIR}/project/build-scripts/list-dir.sh)
add_custom_command(
    OUTPUT "${RESOURCE_LIST_FILE}"
    COMMAND ${LIST_DIR_EXE} -o ${RESOURCE_LIST_FILE} ${CMAKE_BINARY_DIR}/assets
    DEPENDS ${ASSET_TARGETS} ${LIST_DIR_EXE}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/assets"
    COMMENT "generating asset list"
    VERBATIM
)
add_custom_target(resources_txt ALL DEPENDS ${RESOURCE_LIST_FILE})

set(ASSET_HASH_FILE "${CMAKE_BINARY_DIR}/assets/cozmo_assets.ref")
set(HASH_DIR_EXE ${CMAKE_SOURCE_DIR}/project/build-scripts/md5-dir.sh)
add_custom_command(
    OUTPUT ${ASSET_HASH_FILE}
    COMMAND ${HASH_DIR_EXE} -o ${ASSET_HASH_FILE} ${CMAKE_BINARY_DIR}/assets
    DEPENDS ${ASSET_TARGETS} ${RESOURCES_LIST_FILE} ${HASH_DIR_EXE}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/assets"
    COMMENT "generate assets hash file"
    VERBATIM
)
add_custom_target(assets_hash ALL DEPENDS ${ASSET_HASH_FILE})
    
add_dependencies(cozmo_engine
    ${ASSET_TARGETS}
    resources_txt
    #${RESOURCE_LIST_FILE}
    assets_hash
    #${ASSET_HASH_FILE}
)

# calculate assethash
