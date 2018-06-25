function(anki_build_copy_assets)
    set(options "")
    set(oneValueArgs TARGET DEP_TARGET SRCLIST_DIR OUTPUT_DIR RELATIVE_OUTPUT_DIR)
    set(multiValueArgs OUT_SRCS OUT_DSTS OUT_RELATIVE_DSTS)
    cmake_parse_arguments(cpassets "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(_SRCS "")
    set(_DSTS "")

    set(SRCLIST_FILE "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}.srcs.lst")
    set(DSTLIST_FILE "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}.dsts.lst")
    set(SRCDST_ARGS "")
    if (EXISTS "${SRCLIST_FILE}")
        file(STRINGS "${SRCLIST_FILE}" _SRCS)
        set(SRCDST_ARGS --srcdst ${SRCLIST_FILE} ${DSTLIST_FILE})
    endif()

    if (EXISTS "${DSTLIST_FILE}")
        file(STRINGS "${DSTLIST_FILE}" _DSTS)
    endif()

    set(PLATFORM_SRCLIST_FILE "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}_${ANKI_PLATFORM_NAME}.srcs.lst")
    set(PLATFORM_DSTLIST_FILE "${cpassets_SRCLIST_DIR}/${cpassets_TARGET}_${ANKI_PLATFORM_NAME}.dsts.lst")
    if (EXISTS "${PLATFORM_SRCLIST_FILE}")
        file(STRINGS "${PLATFORM_SRCLIST_FILE}" _PLATFORM_SRCS)
        list(APPEND SRCDST_ARGS --srcdst ${PLATFORM_SRCLIST_FILE} ${PLATFORM_DSTLIST_FILE})
    endif()

    if (EXISTS "${PLATFORM_DSTLIST_FILE}")
        file(STRINGS "${PLATFORM_DSTLIST_FILE}" _PLATFORM_DSTS)
    endif()

    set(SRCS ${_SRCS} ${_PLATFORM_SRCS})
    set(DSTS ${_DSTS} ${_PLATFORM_DSTS})

    list(LENGTH SRCS SRCS_COUNT)
    math(EXPR SRCS_COUNT "${SRCS_COUNT} - 1")

    set(INPUT_FILES "")
    set(OUTPUT_FILES "")
    foreach(SRC ${SRCS})
        set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}")
        list(APPEND INPUT_FILES ${SRC_PATH})
    endforeach()
    foreach(DST ${DSTS})
        set(DST_PATH "${cpassets_OUTPUT_DIR}/${DST}")
        list(APPEND OUTPUT_FILES ${DST_PATH})
        if (cpassets_RELATIVE_OUTPUT_DIR)
            list(APPEND OUTPUT_RELATIVE_DSTS ${cpassets_RELATIVE_OUTPUT_DIR}/${DST})
        else()
            list(APPEND OUTPUT_RELATIVE_DSTS ${DST})
        endif()
    endforeach()

    add_custom_command(
        OUTPUT ${OUTPUT_FILES}
        COMMAND ${CMAKE_SOURCE_DIR}/project/build-scripts/copy-assets.py
        ARGS ${SRCDST_ARGS}
            --output_dir "${cpassets_OUTPUT_DIR}"
            --source_dir "${CMAKE_CURRENT_SOURCE_DIR}"
            --cmake ${CMAKE_COMMAND}
        DEPENDS ${INPUT_FILES}
        COMMENT "copying assets for target ${cpassets_TARGET}"
        VERBATIM
    )

    if (NOT ${cpassets_DEP_TARGET})
        set(cpassets_DEP_TARGET ALL)
    endif()

    add_custom_target(${cpassets_TARGET} ALL DEPENDS ${OUTPUT_FILES})

    list(APPEND OUTPUT_RELATIVE_DSTS ${${cpassets_OUT_RELATIVE_DSTS}})
    set(${cpassets_OUT_RELATIVE_DSTS} ${OUTPUT_RELATIVE_DSTS} PARENT_SCOPE)

    list(APPEND OUTPUT_FILES ${${cpassets_OUT_DSTS}})
    set(${cpassets_OUT_DSTS} ${OUTPUT_FILES} PARENT_SCOPE)

    list(APPEND INPUT_FILES ${${cpassets_OUT_SRCS}})
    set(${cpassets_OUT_SRCS} ${INPUT_FILES} PARENT_SCOPE)

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
