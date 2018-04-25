include(anki_build_copy_assets)

list(APPEND ASSET_OUTPUT_RELATIVE_DSTS "")
list(APPEND ASSET_OUTPUT_FILES "")

anki_build_copy_assets(
    TARGET cozmo_resources_assets
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
    RELATIVE_OUTPUT_DIR "cozmo_resources"
    OUT_DSTS ASSET_OUTPUT_FILES
    OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
)

anki_build_copy_assets(
    TARGET cozmo_resources_sound
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
    RELATIVE_OUTPUT_DIR "cozmo_resources"
    OUT_DSTS ASSET_OUTPUT_FILES
    OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
)

anki_build_copy_assets(
    TARGET cozmo_resources_config
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
    RELATIVE_OUTPUT_DIR "cozmo_resources"
    OUT_DSTS ASSET_OUTPUT_FILES
    OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
)

# Factory test shouldn't have webserver resources
if (NOT FACTORY_TEST)
  anki_build_copy_assets(
      TARGET cozmo_resources_webserver
      SRCLIST_DIR ${ANKI_SRCLIST_DIR}
      OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
      RELATIVE_OUTPUT_DIR "cozmo_resources"
      OUT_DSTS ASSET_OUTPUT_FILES
      OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
  )
endif()

anki_build_copy_assets(
    TARGET cozmo_bundle
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets"
    OUT_DSTS ASSET_OUTPUT_FILES
    OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
)

anki_build_copy_assets(
  TARGET        cozmo_resources_tts
  SRCLIST_DIR   ${ANKI_SRCLIST_DIR}
  OUTPUT_DIR   "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
  RELATIVE_OUTPUT_DIR "cozmo_resources"
  OUT_DSTS ASSET_OUTPUT_FILES
  OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
)

set(ASSET_TARGETS
    cozmo_resources_assets
    cozmo_resources_config
    cozmo_resources_webserver
    cozmo_resources_tts
    cozmo_resources_sound
    cozmo_bundle)

# Remove webserver resources from asset targets if we are building
# for the factory test
if (FACTORY_TEST)
   list(REMOVE_ITEM ASSET_TARGETS cozmo_resources_webserver)
endif()

# Bundle test assets on OSX
if (MACOSX)
    anki_build_copy_assets(
        TARGET cozmo_resources_test
        SRCLIST_DIR ${ANKI_SRCLIST_DIR}
        OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
        RELATIVE_OUTPUT_DIR "cozmo_resources"
        OUT_DSTS ASSET_OUTPUT_FILES
        OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
    )
    list(APPEND ASSET_TARGETS cozmo_resources_test)
endif(MACOSX)

# Uncomment when debugging asset list aggregation
# list(LENGTH ASSET_OUTPUT_RELATIVE_DSTS __COUNT)
# message(STATUS "ASSET_OUTPUT_DSTS COUNT : ${__COUNT}")

set (ASSET_METADATA_BASENAME "${CMAKE_BINARY_DIR}/asset-build")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/asset-build.manifest.in
    "${ASSET_METADATA_BASENAME}.manifest"
)

add_custom_command(
    OUTPUT "${ASSET_METADATA_BASENAME}.gc.stamp"
    COMMAND ${CMAKE_SOURCE_DIR}/project/victor/scripts/prune-dir.sh -c
        -o ${ASSET_METADATA_BASENAME}.gc.stamp
        -m ${ASSET_METADATA_BASENAME}.manifest
        ${CMAKE_BINARY_DIR}/data/assets
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS ${ASSET_METADATA_BASENAME}.manifest ${ASSET_OUTPUT_FILES}
    COMMENT "rm stale asset files"
)
add_custom_target(prune_files ALL DEPENDS
    "${ASSET_METADATA_BASENAME}.gc.stamp"
)

add_dependencies(cozmo_engine
    ${ASSET_TARGETS}
    prune_files
)

add_dependencies(victor_anim
    ${ASSET_TARGETS}
    prune_files
)
