# symlink_target
# attach a symlink operation to a target
#
# usage:
# symlink_target(TARGET <cmake target>
#                [SRC <src file or dir>]
#                DST <dst file or dir>
# SRC is optional and if not specified, the target file associated with TARGET will be used.
# symlink_target will attempt to create dst dir trees as necessary
# If DST ends with '/', it is assumed to be a dir and will be created if necessary
# Otherwise DST is assumed to be a file and `dirname ${DST}` will be created if necessary
#
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
        # create dir if necessary
        COMMAND \[\[ "${LN_DST}" = *\/ \]\] && mkdir -p ${LN_DST} || mkdir -p $$\(dirname ${LN_DST}\)
        COMMAND ln -Ffhs ${LN_SRC} ${LN_DST}
        COMMENT "symlink ${LN_DST} -> ${LN_SRC}"
    )
endfunction()
