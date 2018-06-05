function(anki_build_copy_assets)
    set(options "")
    set(oneValueArgs TARGET DEP_TARGET SRCLIST_DIR OUTPUT_DIR RELATIVE_OUTPUT_DIR)
    set(multiValueArgs OUT_SRCS OUT_DSTS OUT_RELATIVE_DSTS)
    cmake_parse_arguments(cpassets "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(_SRCS "")
    set(_DSTS "")

    if (EXISTS "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}.srcs.lst")
        file(STRINGS "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}.srcs.lst" _SRCS)
    endif()

    if (EXISTS "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}.dsts.lst")
        file(STRINGS "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}.dsts.lst" _DSTS)
    endif()

    if (EXISTS "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}_${ANKI_PLATFORM_NAME}.srcs.lst")
        file(STRINGS "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}_${ANKI_PLATFORM_NAME}.srcs.lst" _PLATFORM_SRCS)
    endif()

    if (EXISTS "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}_${ANKI_PLATFORM_NAME}.dsts.lst")
        file(STRINGS "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}_${ANKI_PLATFORM_NAME}.dsts.lst" _PLATFORM_DSTS)
    endif()

    set(SRCS ${_SRCS} ${_PLATFORM_SRCS})
    set(DSTS ${_DSTS} ${_PLATFORM_DSTS})

    list(LENGTH SRCS SRCS_COUNT)
    math(EXPR SRCS_COUNT "${SRCS_COUNT} - 1")

    set(INPUT_FILES "")
    set(OUTPUT_FILES "")
    foreach(IDX RANGE ${SRCS_COUNT})
        list(GET SRCS ${IDX} SRC)
        list(GET DSTS ${IDX} DST)

        # uncomment for DEBUGGING
        # message(STATUS "cp ${SRC} ${DST}")

        set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}")
        set(DST_PATH "${cpassets_OUTPUT_DIR}/${DST}")

        add_custom_command(
            OUTPUT ${DST_PATH}
            COMMAND ${CMAKE_COMMAND}
            ARGS -E copy_if_different "${SRC_PATH}" "${DST_PATH}"
            DEPENDS ${SRC_PATH}
            COMMENT "cp ${SRC_PATH} ${DST_PATH}"
            VERBATIM
        )
        list(APPEND OUTPUT_FILES ${DST_PATH})
        list(APPEND INPUT_FILES ${SRC_PATH})

        if (cpassets_RELATIVE_OUTPUT_DIR)
            list(APPEND OUTPUT_RELATIVE_DSTS ${cpassets_RELATIVE_OUTPUT_DIR}/${DST})
        else()
            list(APPEND OUTPUT_RELATIVE_DSTS ${DST})
        endif()
    endforeach()

    list(APPEND OUTPUT_RELATIVE_DSTS ${${cpassets_OUT_RELATIVE_DSTS}})
    set(${cpassets_OUT_RELATIVE_DSTS} ${OUTPUT_RELATIVE_DSTS} PARENT_SCOPE)

    list(APPEND OUTPUT_FILES ${${cpassets_OUT_DSTS}})
    set(${cpassets_OUT_DSTS} ${OUTPUT_FILES} PARENT_SCOPE)

    list(APPEND INPUT_FILES ${${cpassets_OUT_SRCS}})
    set(${cpassets_OUT_SRCS} ${INPUT_FILES} PARENT_SCOPE)

    if (NOT ${cpassets_DEP_TARGET})
        set(cpassets_DEP_TARGET ALL)
    endif()

    add_custom_target(${cpassets_TARGET} ALL DEPENDS ${OUTPUT_FILES})
endfunction()

function(anki_build_copy_assets_py)
    set(options "")
    set(oneValueArgs TARGET DEP_TARGET SRCLIST_DIR OUTPUT_DIR RELATIVE_OUTPUT_DIR)
    set(multiValueArgs OUT_SRCS OUT_DSTS OUT_RELATIVE_DSTS)
    cmake_parse_arguments(cpassets "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # do the work to generate the input for the manifest for prune_files

    set(_SRCS "")
    set(_DSTS "")

    if (EXISTS "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}.srcs.lst")
        file(STRINGS "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}.srcs.lst" _SRCS)
    endif()

    if (EXISTS "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}.dsts.lst")
        file(STRINGS "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}.dsts.lst" _DSTS)
    endif()

    if (EXISTS "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}_${ANKI_PLATFORM_NAME}.srcs.lst")
        file(STRINGS "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}_${ANKI_PLATFORM_NAME}.srcs.lst" _PLATFORM_SRCS)
    endif()

    if (EXISTS "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}_${ANKI_PLATFORM_NAME}.dsts.lst")
        file(STRINGS "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}_${ANKI_PLATFORM_NAME}.dsts.lst" _PLATFORM_DSTS)
    endif()

    set(SRCS ${_SRCS} ${_PLATFORM_SRCS})
    set(DSTS ${_DSTS} ${_PLATFORM_DSTS})

    list(LENGTH SRCS SRCS_COUNT)
    math(EXPR SRCS_COUNT "${SRCS_COUNT} - 1")

    set(INPUT_FILES "")
    set(OUTPUT_FILES "")
    foreach(IDX RANGE ${SRCS_COUNT})
        list(GET SRCS ${IDX} SRC)
        list(GET DSTS ${IDX} DST)

        # uncomment for DEBUGGING
        # message(STATUS "cp ${SRC} ${DST}")

        set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}")
        set(DST_PATH "${cpassets_OUTPUT_DIR}/${DST}")

        list(APPEND OUTPUT_FILES ${DST_PATH})
        list(APPEND INPUT_FILES ${SRC_PATH})

        if (cpassets_RELATIVE_OUTPUT_DIR)
            list(APPEND OUTPUT_RELATIVE_DSTS ${cpassets_RELATIVE_OUTPUT_DIR}/${DST})
        else()
            list(APPEND OUTPUT_RELATIVE_DSTS ${DST})
        endif()
    endforeach()

    list(APPEND OUTPUT_RELATIVE_DSTS ${${cpassets_OUT_RELATIVE_DSTS}})
    set(${cpassets_OUT_RELATIVE_DSTS} ${OUTPUT_RELATIVE_DSTS} PARENT_SCOPE)

    list(APPEND OUTPUT_FILES ${${cpassets_OUT_DSTS}})
    set(${cpassets_OUT_DSTS} ${OUTPUT_FILES} PARENT_SCOPE)

    list(APPEND INPUT_FILES ${${cpassets_OUT_SRCS}})
    set(${cpassets_OUT_SRCS} ${INPUT_FILES} PARENT_SCOPE)

    if (NOT ${cpassets_DEP_TARGET})
        set(cpassets_DEP_TARGET ALL)
    endif()

    # this target will always run

    add_custom_target(
        ${cpassets_TARGET}
        ALL
    )
    add_custom_command(
        TARGET ${cpassets_TARGET}
        PRE_BUILD
        COMMAND python2 ${CMAKE_SOURCE_DIR}/python/anki_build_copy_assets.py
        --target ${cpassets_TARGET}
        --srclist_dir ${cpassets_SRCLIST_DIR}
        --output_dir ${cpassets_OUTPUT_DIR}
        --anki_platform_name ${ANKI_PLATFORM_NAME}
        --cmake_current_source_dir ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "copying for ${cpassets_TARGET}"
    )

endfunction()

function(anki_build_copydirectory_assets)
    set(options "")
    set(oneValueArgs TARGET DEP_TARGET SRCLIST_DIR OUTPUT_DIR RELATIVE_OUTPUT_DIR)
    set(multiValueArgs OUT_SRCS OUT_DSTS OUT_RELATIVE_DSTS)
    cmake_parse_arguments(cpassets "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    file(GLOB_RECURSE SRCS
         LIST_DIRECTORIES false
         RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/${cpassets_SRCLIST_DIR}
         ${CMAKE_CURRENT_SOURCE_DIR}/${cpassets_SRCLIST_DIR}/*)

    foreach(src ${SRCS})
        list(APPEND INPUT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${cpassets_SRCLIST_DIR}/${src})
        list(APPEND OUTPUT_FILES ${cpassets_OUTPUT_DIR}/${src})
        list(APPEND OUTPUT_RELATIVE_DSTS ${cpassets_RELATIVE_OUTPUT_DIR}/${src})
    endforeach()

    add_custom_command(
        COMMENT "Copying ${cpassets_SRCLIST_DIR} to ${cpassets_OUTPUT_DIR}"
        OUTPUT ${OUTPUT_FILES}
        DEPENDS ${INPUT_FILES}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/${cpassets_SRCLIST_DIR} ${cpassets_OUTPUT_DIR}
    )

    add_custom_target(
        ${cpassets_TARGET} ALL
        DEPENDS ${INPUT_FILES} ${OUTPUT_FILES}
    )

    list(APPEND OUTPUT_RELATIVE_DSTS ${${cpassets_OUT_RELATIVE_DSTS}})
    set(${cpassets_OUT_RELATIVE_DSTS} ${OUTPUT_RELATIVE_DSTS} PARENT_SCOPE)

    list(APPEND OUTPUT_FILES ${${cpassets_OUT_DSTS}})
    set(${cpassets_OUT_DSTS} ${OUTPUT_FILES} PARENT_SCOPE)

    list(APPEND INPUT_FILES ${${cpassets_OUT_SRCS}})
    set(${cpassets_OUT_SRCS} ${INPUT_FILES} PARENT_SCOPE)
endfunction()
