
include(anki_build_util)

macro(anki_build_go_c_library target_name gensrc_var srclist_dir)
  __anki_build_absolute_source_list(${target_name} ${srclist_dir})

  set(extra_argv ${ARGN})
  list(LENGTH extra_argv extra_argc)

  if (extra_argc GREATER 0)
    # process extra args here
  endif()

  # set the locations of the .a and .h files that will be generated
  set(_gobuild_out "${CMAKE_CURRENT_BINARY_DIR}/${target_name}/${target_name}.a")
  get_filename_component(_gobuild_out_dir ${_gobuild_out} DIRECTORY)
  set(_gobuild_header "${_gobuild_out_dir}/${target_name}.h")

  # get the source dir - must be named "go"
  # this is needed so we can `go get` any dependencies from the source
  list(GET SRCS 0 _gobuild_first_src)
  string(FIND ${_gobuild_first_src} "/go/" _gobuild_srcdir_idx REVERSE)
  string(SUBSTRING ${_gobuild_first_src} 0 ${_gobuild_srcdir_idx} _gobuild_srcdir)
  set(_gobuild_srcdir "${_gobuild_srcdir}/go")

  # get the source path relative to our working directory - `go get` seems
  # to barf with absolute pathnames
  file(RELATIVE_PATH _gobuild_srcdir_rel ${CMAKE_CURRENT_BINARY_DIR} "${_gobuild_srcdir}")

  # build our go code - first `go get` dependencies, then `go build` a c-archive with our source files
  add_custom_command(
      OUTPUT ${_gobuild_out} ${_gobuild_header}
      COMMAND go get -d ${_gobuild_srcdir_rel}
      COMMAND go build -buildmode=c-archive -o ${_gobuild_out} ${SRCS} ${_ab_PLATFORM_SRCS}
      DEPENDS ${SRCS} ${_ab_PLATFORM_SRCS}
  )

  # export the location of the generated C header:
  set(${gensrc_var} ${_gobuild_header})

  # and lastly, some stuff I don't understand at all to export {target}_out as a library
  # that other projects can link to:
  add_custom_target(_gobuild_target DEPENDS ${_gobuild_out})
  add_library(${target_name} STATIC IMPORTED GLOBAL)
  add_dependencies(${target_name} _gobuild_target)
  set_property(TARGET ${target_name} PROPERTY IMPORTED_LOCATION ${_gobuild_out})

endmacro()

# build a go executable
# usage: anki_build_go_executable(mytarget "path/to/source/directory" "path/to/GOPATH/directory" ${ANKI_SRCLIST_DIR})
macro(anki_build_go_executable target_name target_basedir gopath srclist_dir)
  __anki_build_absolute_source_list(${target_name} ${srclist_dir})

  set(__gobuild_out "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}")
  get_filename_component(__gobuild_gopath "${gopath}" ABSOLUTE)
  get_filename_component(__gobuild_basedir "${target_basedir}" ABSOLUTE)
  file(RELATIVE_PATH __gobuild_basedir ${CMAKE_CURRENT_BINARY_DIR} "${__gobuild_basedir}")

  set(__go_get_env "GOPATH=${__gobuild_gopath}")
  set(__go_build_flags "")
  set(__go_deps "")

  list(APPEND __go_build_flags "-i")
  list(APPEND __go_build_flags "-o" "${__gobuild_out}")

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
    list(APPEND __go_compile_env "CGO_ENABLED=1")
    list(APPEND __go_compile_env "CGO_FLAGS=\"-march=armv7-a\"")
    set(__gobuild_pkgdir )
    list(APPEND __go_build_flags "-pkgdir" "${CMAKE_CURRENT_BINARY_DIR}/pkgdir")
    list(APPEND __go_deps android_toolchain)
  endif()

  # `go build` should share `go get` environment
  list(APPEND __go_compile_env "${__go_get_env}")

  add_custom_command(
    OUTPUT ${__gobuild_out}
    COMMAND ${CMAKE_COMMAND} -E env ${__go_get_env} go get -d ${__gobuild_basedir}
    COMMAND ${CMAKE_COMMAND} -E env ${__go_compile_env} go build ${__go_build_flags} ${SRCS} ${_ab_PLATFORM_SRCS}
    DEPENDS ${SRCS} ${_ab_PLATFORM_SRCS} ${__go_deps}
  )
  add_custom_target(${target_name} DEPENDS ${__gobuild_out})

endmacro()
