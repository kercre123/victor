function(vicos_strip)
    # Skip separate strip step if not on VICOS
    if (NOT VICOS)
      return()
    endif()

    set(options "")
    set(oneValueArgs TARGET)
    set(multiValueArgs "")
    cmake_parse_arguments(astrip "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    get_property(TARGET_TYPE TARGET ${astrip_TARGET} PROPERTY TYPE)

    if (${TARGET_TYPE} STREQUAL "STATIC_LIBRARY")
        return()
    endif()

    set(STRIP_CMD "${VICOS_TOOLCHAIN_PREFIX}strip")
    set(OBJCOPY_CMD "${VICOS_TOOLCHAIN_PREFIX}objcopy")
    add_custom_command(TARGET ${astrip_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:${astrip_TARGET}>
            $<TARGET_FILE:${astrip_TARGET}>.full
        COMMAND ${STRIP_CMD} --strip-unneeded $<TARGET_FILE:${astrip_TARGET}>
        COMMAND ${OBJCOPY_CMD} --add-gnu-debuglink
            $<TARGET_FILE:${astrip_TARGET}>.full
            $<TARGET_FILE:${astrip_TARGET}>
        COMMAND ${CMAKE_COMMAND} -E touch_nocreate $<TARGET_FILE:${astrip_TARGET}>.full
        COMMENT "strip lib $<TARGET_FILE:${astrip_TARGET}> -> ${OUTPUT_TARGET_NAME}"
        VERBATIM
    )
endfunction()
