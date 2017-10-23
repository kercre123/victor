
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
