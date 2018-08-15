
include(anki_build_source_list)

set(ANKI_GO_VERSION 1.10.2)

# Check correct Go version
execute_process(COMMAND ${GOROOT}/bin/go version
  OUTPUT_VARIABLE GO_VERSION_OUTPUT)
string(REGEX MATCH "[0-9]+\.[0-9]+(\.[0-9]+)?"
  GO_VERSION_MATCH ${GO_VERSION_OUTPUT})
if (NOT ${GO_VERSION_MATCH} STREQUAL ${ANKI_GO_VERSION})
  message(FATAL_ERROR "Detected version of Go is ${GO_VERSION_MATCH}, but we want ${ANKI_GO_VERSION}\n"
    "This should be set correctly by build scripts, maybe you need to reconfigure (with -f)?")
endif()

# Write Go version to file to use as dependency (only updates if version has changed)
set(GO_VERSION_FILE ${CMAKE_BINARY_DIR}/goversion)
set(__GO_VERSION_TMP ${CMAKE_BINARY_DIR}/goversion_tmp)
file(WRITE ${__GO_VERSION_TMP} ${ANKI_GO_VERSION})
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${__GO_VERSION_TMP} ${GO_VERSION_FILE})
file(REMOVE ${__GO_VERSION_TMP})

# internal use - set up build environment for go, based on platform
# vars that should already be set up: __gobuild_out
macro(__anki_setup_go_environment target_basedir)

  get_filename_component(__gobuild_basedir "${CMAKE_SOURCE_DIR}/${target_basedir}" ABSOLUTE)
  file(RELATIVE_PATH __gobuild_basedir ${CMAKE_CURRENT_BINARY_DIR} "${__gobuild_basedir}")

  set(__go_compile_env "CGO_ENABLED=1")
  list(APPEND __go_compile_env "GOCACHE=off")
  list(APPEND __go_compile_env "GOPATH=${GOPATH}")
  set(__go_build_flags "")
  set(__go_deps "")

  list(GET __gobuild_out 0 __gobuild_primary_out)

  list(APPEND __go_build_flags "-o" "${__gobuild_primary_out}")

  if (ANKI_DEV_CHEATS EQUAL 0)
    list(APPEND __go_build_flags "-tags" "'shipping'")
  endif()

  if (VICOS)
    # set vicos flags for `go build`
    list(APPEND __go_compile_env "GOOS=linux")
    list(APPEND __go_compile_env "GOARCH=arm")
    list(APPEND __go_compile_env "GOARM=7")
    list(APPEND __go_compile_env "CC=${VICOS_C_COMPILER}")
    list(APPEND __go_compile_env "CXX=${VICOS_CXX_COMPILER}")
    list(APPEND __go_compile_env "CGO_FLAGS=\"-g -march=armv7-a\"")
    list(APPEND __go_build_flags "-pkgdir" "${CMAKE_CURRENT_BINARY_DIR}/pkgdir")
  endif()
endmacro()

# internal use - execute go build with the environment
# set up in `__go_compile_env`, `__go_build_flags`, and `__go_deps`
macro(__anki_run_go_build target_name extra_deps)

  set(__include_dirs $<TARGET_PROPERTY:${target_name}_fake_dep,INCLUDE_DIRECTORIES>)
  set(__link_libs $<TARGET_PROPERTY:${target_name}_fake_dep,LINK_LIBRARIES>)
  set(__link_folders $<TARGET_PROPERTY:${target_name},GO_CLINK_FOLDERS>)
  set(__cgo_cppflags $<TARGET_PROPERTY:${target_name},CGO_CPPFLAGS>)
  set(__cgo_ldflags $<TARGET_PROPERTY:${target_name},CGO_LDFLAGS>)

  set(__cgo_cppflags "${__cgo_cppflags} $<$<BOOL:${__include_dirs}>:-I $<JOIN:${__include_dirs}, -I >>")
  set(__cgo_ldflags "${__cgo_ldflags} ${__link_folders}\ $<$<BOOL:${__link_libs}>:-l$<JOIN:${__link_libs},\ -l>>")
  set(__cgo_cxxflags "-std=c++14")
  if (VICOS)
    set(__cgo_cxxflags "${__cgo_cxxflags} -stdlib=libc++")
    set(__cgo_ldflags "${__cgo_ldflags} -stdlib=libc++")
  endif()

  set(__cppflags_env "CGO_CPPFLAGS=${__cgo_cppflags}")
  set(__cxxflags_env "CGO_CXXFLAGS=${__cgo_cxxflags}")
  set(__ldflags_env "CGO_LDFLAGS=${__cgo_ldflags}")

  set(__go_platform_ldflags "")
  if (VICOS)
    set(__go_platform_ldflags "-r /anki/lib")
  endif()
  set(__go_build_ldflags $<TARGET_PROPERTY:${target_name},GO_LDFLAGS>\ ${__go_platform_ldflags})
  set(__ldflags_str $<$<BOOL:${__go_build_ldflags}>:-ldflags>)

  add_custom_command(
    OUTPUT ${__gobuild_out}
    COMMAND ${CMAKE_COMMAND} -E env ${__go_compile_env} ${__cppflags_env} ${__ldflags_env} ${__cxxflags_env}
                             ${GOROOT}/bin/go build ${__go_build_flags}
                             ${__ldflags_str} ${__go_build_ldflags}
                             ${__gobuild_basedir}
    DEPENDS ${SRCS} ${_ab_PLATFORM_SRCS} ${__go_deps} ${extra_deps} ${GO_VERSION_FILE}
  )
endmacro()

# set up a fake dependency that won't be built but we can query for dependency properties;
# by telling this fake target to link against other libraries, we can figure out what include
# folders and link targets we need to pass to the go build
macro(__anki_build_go_fake_target target_name)
  if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/__dummy.c")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/__dummy.c" "")
  endif()
  add_library(${target_name}_fake_dep "${CMAKE_CURRENT_BINARY_DIR}/__dummy.c")
  define_property(TARGET PROPERTY GO_CLINK_FOLDERS BRIEF_DOCS "a" FULL_DOCS "b")
  define_property(TARGET PROPERTY GO_LDFLAGS BRIEF_DOCS "a" FULL_DOCS "b")
  define_property(TARGET PROPERTY CGO_CPPFLAGS BRIEF_DOCS "a" FULL_DOCS "b")
  define_property(TARGET PROPERTY CGO_LDFLAGS BRIEF_DOCS "a" FULL_DOCS "b")
  set_target_properties(${target_name}_fake_dep PROPERTIES EXCLUDE_FROM_ALL TRUE
                                                           INCLUDE_DIRECTORIES "")
  anki_build_target_license(${target_name}_fake_dep "ANKI")
endmacro()

#
# Helper macro to strip an object file (exe or lib) and store debug symbols
# into a ".full" file.  This macro assumes that CMAKE_STRIP and CMAKE_OBJCOPY
# are set appropriately for the current toolchain.
#
# The symbol file depends on the object file, so it will be rebuilt
# after any change to the object file. We follow the strip operation
# with a touch operation to ensure that the symbol file is newer
# than the stripped executable.
#
# These commands duplicate logic in android_strip.cmake, but they are
# repackaged to work with custom go targets.
#

macro(anki_build_go_vicos_strip output output_full)
  add_custom_command(
    OUTPUT ${output_full}
    DEPENDS ${output}
    COMMAND ${CMAKE_COMMAND} -E copy ${output} ${output_full}
    COMMAND ${CMAKE_STRIP} --strip-unneeded ${output}
    COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink ${output_full} ${output}
    COMMAND ${CMAKE_COMMAND} -E touch_nocreate ${output_full}
    COMMENT strip ${output} to create ${output_full}
    VERBATIM
  )
endmacro()

# build a go library that can be linked in with C code later
# gensrc_var is a variable name where the generated header (that defines exported functions
# from the library) location will be stored
# usage: anki_build_go_c_library(mytarget generated_header_variable "path/to/source/directory"
#                                "path/to/GOPATH/directory" ${ANKI_SRCLIST_DIR})
macro(anki_build_go_c_library target_name gensrc_var srclist_dir extra_deps)
  anki_build_absolute_source_list(${target_name} ${srclist_dir})

  # set the locations of the .a and .h files that will be generated
  set(__gobuild_out "${CMAKE_CURRENT_BINARY_DIR}/${target_name}/${target_name}.a")
  get_filename_component(__gobuild_out_dir ${__gobuild_out} DIRECTORY)
  set(__gobuild_header "${__gobuild_out_dir}/${target_name}.h")

  # add header to output list
  list(APPEND __gobuild_out ${__gobuild_header})

  __anki_setup_go_environment(${_ab_GO_DIR})

  # add our c-library-specific build flag
  # TODO: can't build archives for android/arm, need to either try linux/arm (need a toolchain for that also?)
  # or build shared libraries instead on android (need to then deal with an additional output we didn't expect)
  list(INSERT __go_build_flags 0 "-buildmode=c-archive")

  # some stuff I don't understand at all to export {target}_out as a library
  # that other projects can link to:
  add_library(${target_name} STATIC IMPORTED GLOBAL)
  add_dependencies(${target_name} ${__gobuild_out})
  set_property(TARGET ${target_name} PROPERTY IMPORTED_LOCATION ${__gobuild_primary_out})

  __anki_build_go_fake_target(${target_name})

  set_target_properties(${target_name} PROPERTIES GO_CLINK_FOLDERS "")

  __anki_run_go_build(${target_name} "${extra_deps}")

  # export the location of the generated C header:
  set(${gensrc_var} ${__gobuild_header})

endmacro()

# build a go executable
# usage: anki_build_go_executable(mytarget "path/to/source/directory" "path/to/GOPATH/directory" ${ANKI_SRCLIST_DIR})

macro(anki_build_go_executable target_name srclist_dir extra_deps)
  anki_build_absolute_source_list(${target_name} ${srclist_dir})

  set(__gobuild_out "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}")
  set(__gobuild_out_full "")

  #
  # On vicos, generate additional commands to strip executable.
  # Debug symbols are stored in a ".full" file. The build target
  # depends on both exe and exe.full so they will be created together.
  #
  if (VICOS)
    set(__gobuild_out_full "${__gobuild_out}.full")
    anki_build_go_vicos_strip(${__gobuild_out} ${__gobuild_out_full})
  endif()

  add_custom_target(${target_name} DEPENDS ${__gobuild_out} ${__gobuild_out_full})

  __anki_build_go_fake_target(${target_name})


  set_target_properties(${target_name} PROPERTIES GO_CLINK_FOLDERS ""
                                                  ANKI_OUT_PATH ${__gobuild_out}
                                                  EXCLUDE_FROM_ALL FALSE)

  __anki_setup_go_environment(${_ab_GO_DIR})
  __anki_run_go_build(${target_name} "${extra_deps}")


endmacro()


# specify that a go project should link against another C library
macro(anki_go_add_c_library target_name c_target)
  target_link_libraries(${target_name}_fake_dep PUBLIC ${c_target})
  if (TARGET ${c_target})
    get_target_property(__is_imported ${c_target} IMPORTED)
    if (${__is_imported})
      get_target_property(__c_link_location ${c_target} IMPORTED_LOCATION)
      get_filename_component(__c_link_location ${__c_link_location} DIRECTORY)
    else()
      get_target_property(__c_link_location ${c_target} ARCHIVE_OUTPUT_DIRECTORY)
    endif()
    get_target_property(__current_link_folders ${target_name} GO_CLINK_FOLDERS)
    set(__current_link_folders "${__current_link_folders} -L${__c_link_location}")
    set_property(TARGET ${target_name} PROPERTY GO_CLINK_FOLDERS ${__current_link_folders})
    add_dependencies(${target_name} ${c_target})
  endif()
endmacro()

macro(anki_go_add_include_dir target_name include_dir)
  target_include_directories(${target_name}_fake_dep PUBLIC ${include_dir})
endmacro()

macro(anki_go_set_ldflags target_name flags)
  set_target_properties(${target_name} PROPERTIES GO_LDFLAGS "${flags}")
endmacro()

macro(anki_go_set_cgo_cppflags target_name flags)
  set_target_properties(${target_name} PROPERTIES CGO_CPPFLAGS "${flags}")
endmacro()

macro(anki_go_set_cgo_ldflags target_name flags)
  set_target_properties(${target_name} PROPERTIES CGO_LDFLAGS "${flags}")
endmacro()
