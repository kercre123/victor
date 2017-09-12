# attach a symlink operation to a target
function(symlink_target)
    set(options "")
    set(oneValueArgs TARGET SRC DST)
    set(multiValueArgs "")
    cmake_parse_arguments(lns "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT lns_SRC)
        set(LN_SRC $<TARGET_FILE:${lns_TARGET}>)
    else()
        set(LN_SRC ${lns_SRC})
    endif()

    set(LN_DST ${lns_DST})
    add_custom_command(TARGET ${lns_TARGET} POST_BUILD
        COMMAND \[\[ "${LN_DST}" = *\/ \]\] && mkdir -p "${LN_DST}" || mkdir -p $(dirname "${LN_DST}")
        COMMAND ln -Ffhs ${LN_SRC} ${LN_DST}
        COMMENT "symlink ${LN_SRC} -> ${LN_DST}"
        VERBATIM
    )
endfunction()
