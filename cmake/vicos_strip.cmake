include (anki_build_util)

function(vicos_strip)
    set(options "")
    set(oneValueArgs TARGET)
    set(multiValueArgs "")
    cmake_parse_arguments(astrip "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT VICOS)
        # Skip separate strip step if not on VICOS
        return()
    endif()

    get_property(TARGET_TYPE TARGET ${astrip_TARGET} PROPERTY TYPE)

    if (${TARGET_TYPE} STREQUAL "STATIC_LIBRARY")
        return()
    endif()

    anki_get_output_location(TARGET_OUT_PATH ${astrip_TARGET})

    set(STRIP_CMD "${VICOS_TOOLCHAIN_PREFIX}strip")
    set(OBJCOPY_CMD "${VICOS_TOOLCHAIN_PREFIX}objcopy")
    add_custom_command(TARGET ${astrip_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${TARGET_OUT_PATH}
            ${TARGET_OUT_PATH}.full
        COMMAND ${STRIP_CMD} --strip-unneeded ${TARGET_OUT_PATH}
        COMMAND ${OBJCOPY_CMD} --add-gnu-debuglink
            ${TARGET_OUT_PATH}.full
            ${TARGET_OUT_PATH}
        COMMENT "strip lib ${TARGET_OUT_PATH} -> ${OUTPUT_TARGET_NAME}"
        VERBATIM
    )
endfunction()

