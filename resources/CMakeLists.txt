include(anki_build_copy_assets)

set(ASSET_OUTPUT_RELATIVE_DSTS)
set(ASSET_OUTPUT_FILES)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    anki_build_copy_assets(
        TARGET cozmo_resources_assets
        SRCLIST_DIR ${ANKI_SRCLIST_DIR}
        OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
        RELATIVE_OUTPUT_DIR "cozmo_resources"
        OUT_DSTS ASSET_OUTPUT_FILES
        OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
    )
else()
    anki_build_copy_assets_py(
        TARGET cozmo_resources_assets
        SRCLIST_DIR ${ANKI_SRCLIST_DIR}
        OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
        RELATIVE_OUTPUT_DIR "cozmo_resources"
        OUT_DSTS ASSET_OUTPUT_FILES
        OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
    )
endif()

# Override sound target for locally generated sound banks
set(sound_target cozmo_resources_sound)
option(USE_LOCAL_AUDIO_ASSETS "Use local audio assets" OFF)
if(USE_LOCAL_AUDIO_ASSETS)
    message("----------------------------------------------------------------------")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/externals/local-audio-assets")
        set(sound_target cozmo_resources_local_sound)
        message("Using locally generated audio assets - EXTERNALS/local-audio-assets")
    else()
        message("WARNING - Locally generated audio assets do NOT exists!")
        message("Reverting back to use DEPS assets")
    endif()
    message("----------------------------------------------------------------------")
endif()

if(0)
    anki_build_copy_assets_py(
        TARGET ${sound_target}
        SRCLIST_DIR ${ANKI_SRCLIST_DIR}
        OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
        RELATIVE_OUTPUT_DIR "cozmo_resources"
        OUT_DSTS ASSET_OUTPUT_FILES
        OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    anki_build_copydirectory_assets(
        TARGET ${sound_target}
        SRCLIST_DIR "externals/victor-audio-assets/victor_robot/victor_linux"
        OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources/sound"
        RELATIVE_OUTPUT_DIR "cozmo_resources/sound"
        OUT_DSTS ASSET_OUTPUT_FILES
        OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
    )
elseif(MACOSX)
    anki_build_copy_assets_py(
        TARGET ${sound_target}
        SRCLIST_DIR ${ANKI_SRCLIST_DIR}
        OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
        RELATIVE_OUTPUT_DIR "cozmo_resources"
        OUT_DSTS ASSET_OUTPUT_FILES
        OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
    )
else()
    anki_build_copy_assets_py(
        TARGET ${sound_target}
        SRCLIST_DIR ${ANKI_SRCLIST_DIR}
        OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
        RELATIVE_OUTPUT_DIR "cozmo_resources"
        OUT_DSTS ASSET_OUTPUT_FILES
        OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
    )
endif()
add_dependencies(${sound_target} cozmo_resources_assets)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    anki_build_copy_assets(
        TARGET cozmo_resources_config
        SRCLIST_DIR ${ANKI_SRCLIST_DIR}
        OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
        RELATIVE_OUTPUT_DIR "cozmo_resources"
        OUT_DSTS ASSET_OUTPUT_FILES
        OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
    )
else()
    anki_build_copy_assets_py(
        TARGET cozmo_resources_config
        SRCLIST_DIR ${ANKI_SRCLIST_DIR}
        OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
        RELATIVE_OUTPUT_DIR "cozmo_resources"
        OUT_DSTS ASSET_OUTPUT_FILES
        OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
    )
endif()
add_dependencies(cozmo_resources_config ${sound_target})

# Factory test shouldn't have webserver resources
if (NOT FACTORY_TEST)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        anki_build_copy_assets(
            TARGET cozmo_resources_webserver
            SRCLIST_DIR ${ANKI_SRCLIST_DIR}
            OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
            RELATIVE_OUTPUT_DIR "cozmo_resources"
            OUT_DSTS ASSET_OUTPUT_FILES
            OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
        )
    else()
        anki_build_copy_assets_py(
            TARGET cozmo_resources_webserver
            SRCLIST_DIR ${ANKI_SRCLIST_DIR}
            OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
            RELATIVE_OUTPUT_DIR "cozmo_resources"
            OUT_DSTS ASSET_OUTPUT_FILES
            OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
        )
    endif()
    add_dependencies(cozmo_resources_webserver cozmo_resources_config)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    anki_build_copy_assets(
        TARGET cozmo_bundle
        SRCLIST_DIR ${ANKI_SRCLIST_DIR}
        OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets"
        OUT_DSTS ASSET_OUTPUT_FILES
        OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
    )
else()
    anki_build_copy_assets_py(
        TARGET cozmo_bundle
        SRCLIST_DIR ${ANKI_SRCLIST_DIR}
        OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets"
        OUT_DSTS ASSET_OUTPUT_FILES
        OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
    )
endif()
add_dependencies(cozmo_bundle cozmo_resources_webserver)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    anki_build_copy_assets(
      TARGET        cozmo_resources_tts
      SRCLIST_DIR   ${ANKI_SRCLIST_DIR}
      OUTPUT_DIR   "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
      RELATIVE_OUTPUT_DIR "cozmo_resources"
      OUT_DSTS ASSET_OUTPUT_FILES
      OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
    )
else()
    anki_build_copy_assets_py(
      TARGET        cozmo_resources_tts
      SRCLIST_DIR   ${ANKI_SRCLIST_DIR}
      OUTPUT_DIR   "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
      RELATIVE_OUTPUT_DIR "cozmo_resources"
      OUT_DSTS ASSET_OUTPUT_FILES
      OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
    )
endif()
add_dependencies(cozmo_resources_tts cozmo_bundle)

set(ASSET_TARGETS
    cozmo_resources_assets
    cozmo_resources_config
    cozmo_resources_webserver
    cozmo_resources_tts
    ${sound_target}
    cozmo_bundle)

# Remove webserver resources from asset targets if we are building
# for the factory test
if (FACTORY_TEST)
   list(REMOVE_ITEM ASSET_TARGETS cozmo_resources_webserver)
endif()

# Bundle test assets on OSX
if (MACOSX)
    anki_build_copy_assets_py(
        TARGET cozmo_resources_test
        SRCLIST_DIR ${ANKI_SRCLIST_DIR}
        OUTPUT_DIR "${CMAKE_BINARY_DIR}/data/assets/cozmo_resources"
        RELATIVE_OUTPUT_DIR "cozmo_resources"
        OUT_DSTS ASSET_OUTPUT_FILES
        OUT_RELATIVE_DSTS ASSET_OUTPUT_RELATIVE_DSTS
    )
    add_dependencies(cozmo_resources_test cozmo_resources_tts)
    list(APPEND ASSET_TARGETS cozmo_resources_test)
endif(MACOSX)

# Uncomment when debugging asset list aggregation
# list(LENGTH ASSET_OUTPUT_RELATIVE_DSTS __COUNT)
# message(STATUS "ASSET_OUTPUT_DSTS COUNT : ${__COUNT}")

set (ASSET_METADATA_BASENAME "${CMAKE_BINARY_DIR}/asset-build")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/asset-build.manifest.in
    "${ASSET_METADATA_BASENAME}.manifest"
)

add_custom_target(prune_files ALL
    DEPENDS ${ASSET_TARGETS}
)

add_custom_command(
    TARGET prune_files
    COMMAND ${CMAKE_SOURCE_DIR}/project/victor/scripts/prune-dir.sh -c
        -o ${ASSET_METADATA_BASENAME}.gc.stamp
        -m ${ASSET_METADATA_BASENAME}.manifest
        ${CMAKE_BINARY_DIR}/data/assets
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "rm stale asset files"
)

add_dependencies(cozmo_engine    
    ${ASSET_TARGETS}    
)    
    
add_dependencies(victor_anim    
    ${ASSET_TARGETS}    
)
