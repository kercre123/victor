#~/Developer/android-ndk/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/

#set(ANDROID_TOOLCHAIN_ROOT "${ANDROID_NDK}/toolchains/${ANDROID_TOOLCHAIN_ROOT}-4.9/prebuilt/${ANDROID_HOST_TAG}")
#set(ANDROID_TOOLCHAIN_PREFIX "${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLCHAIN_NAME}-")            
#


# strip -v -p --strip-unneeded -o output.so input.so
function(android_strip)
    set(options "")
    set(oneValueArgs TARGET)
    set(multiValueArgs OUTPUT_LIBS)
    cmake_parse_arguments(astrip "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ANDROID)
        # Skip separate strip step if not on Android
        return()
    endif()

    set(STRIP_CMD "${ANDROID_TOOLCHAIN_PREFIX}strip")

    set(ORIGINAL_TARGET_NAME "$<TARGET_FILE_NAME:${astrip_TARGET}>")
    set_target_properties(${astrip_TARGET} PROPERTIES OUTPUT_NAME "${astrip_TARGET}.full")

    add_custom_command(TARGET ${astrip_TARGET} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy 
            $<TARGET_FILE:${astrip_TARGET}>
            $<TARGET_FILE_DIR:${astrip_TARGET}>/${ORIGINAL_TARGET_NAME}
        COMMENT "strip lib ${OUTPUT_TARGET_NAME}")
endfunction()









