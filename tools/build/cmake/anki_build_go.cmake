
include(anki_build_source_list)

# internal use - set up build environment for go, based on platform
# vars that should already be set up: __gobuild_out
macro(__anki_setup_go_environment target_basedir gopath)

  get_filename_component(__gobuild_gopath "${gopath}" ABSOLUTE)
  get_filename_component(__gobuild_basedir "${target_basedir}" ABSOLUTE)
  file(RELATIVE_PATH __gobuild_basedir ${CMAKE_CURRENT_BINARY_DIR} "${__gobuild_basedir}")

  set(__go_get_env "GOPATH=${__gobuild_gopath}")
  list(APPEND __go_get_env "CGO_ENABLED=1")
  set(__go_build_flags "")
  set(__go_deps "")

  list(GET __gobuild_out 0 __gobuild_primary_out)

  list(APPEND __go_build_flags "-i")
  list(APPEND __go_build_flags "-o" "${__gobuild_primary_out}")

  if(ANDROID)
    # set android flags for `go get`
    list(APPEND __go_get_env "GOOS=android")
    list(APPEND __go_get_env "GOARCH=arm")
    list(APPEND __go_get_env "GOARM=7")
  endif()

  if (ANDROID)
    # set android flags for `go build`
    list(APPEND __go_compile_env "CC=${ANDROID_TOOLCHAIN_CC}")
    list(APPEND __go_compile_env "CXX=${ANDROID_TOOLCHAIN_CXX}")
    list(APPEND __go_compile_env "CGO_FLAGS=\"-march=armv7-a\"")
    list(APPEND __go_build_flags "-pkgdir" "${CMAKE_CURRENT_BINARY_DIR}/pkgdir")
    list(APPEND __go_deps android_toolchain)
  endif()

  # `go build` should share `go get` environment
  list(APPEND __go_compile_env "${__go_get_env}")
endmacro()

# internal use - execute go build (`go get` and `go build`) with the environment
# set up in `__go_compile_env`, `__go_get_env`, `__go_build_flags`, and `__go_deps`
macro(__anki_run_go_build)
  add_custom_command(
    OUTPUT ${__gobuild_out}
    COMMAND ${CMAKE_COMMAND} -E env ${__go_get_env} go get -d ${__gobuild_basedir}
    COMMAND ${CMAKE_COMMAND} -E env ${__go_compile_env} go build ${__go_build_flags} ${SRCS} ${_ab_PLATFORM_SRCS}
    DEPENDS ${SRCS} ${_ab_PLATFORM_SRCS} ${__go_deps} ${_ab_GO_DEPS}
  )
endmacro()

# build a go library that can be linked in with C code later
# gensrc_var is a variable name where the generated header (that defines exported functions
# from the library) location will be stored
# usage: anki_build_go_c_library(mytarget generated_header_variable "path/to/source/directory"
#                                "path/to/GOPATH/directory" ${ANKI_SRCLIST_DIR})
macro(anki_build_go_c_library target_name gensrc_var target_basedir gopath srclist_dir)
  anki_build_absolute_source_list(${target_name} ${srclist_dir})

  # set the locations of the .a and .h files that will be generated
  set(__gobuild_out "${CMAKE_CURRENT_BINARY_DIR}/${target_name}/${target_name}.a")
  get_filename_component(__gobuild_out_dir ${__gobuild_out} DIRECTORY)
  set(__gobuild_header "${__gobuild_out_dir}/${target_name}.h")

  # add header to output list
  list(APPEND __gobuild_out ${__gobuild_header})

  __anki_setup_go_environment(${target_basedir} ${gopath})

  # add our c-library-specific build flag
  # TODO: can't build archives for android/arm, need to either try linux/arm (need a toolchain for that also?)
  # or build shared libraries instead on android (need to then deal with an additional output we didn't expect)
  list(INSERT __go_build_flags 0 "-buildmode=c-archive")

  __anki_run_go_build()

  # export the location of the generated C header:
  set(${gensrc_var} ${__gobuild_header})

  # and lastly, some stuff I don't understand at all to export {target}_out as a library
  # that other projects can link to:
  add_custom_target(__gobuild_${target_name}_target DEPENDS ${__gobuild_out})
  add_library(${target_name} STATIC IMPORTED GLOBAL)
  add_dependencies(${target_name} __gobuild_${target_name}_target)
  set_property(TARGET ${target_name} PROPERTY IMPORTED_LOCATION ${__gobuild_primary_out})

endmacro()

# build a go executable
# usage: anki_build_go_executable(mytarget "path/to/source/directory" "path/to/GOPATH/directory" ${ANKI_SRCLIST_DIR})
macro(anki_build_go_executable target_name target_basedir gopath srclist_dir)
  anki_build_absolute_source_list(${target_name} ${srclist_dir})

  set(__gobuild_out "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}")

  __anki_setup_go_environment(${target_basedir} ${gopath})
  __anki_run_go_build()

  add_custom_target(${target_name} DEPENDS ${__gobuild_out})

endmacro()
