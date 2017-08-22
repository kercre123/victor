function(import target src_subdir)
  if(NOT TARGET ${target})
    message(STATUS "target: " ${target} " " ${src_subdir})
    # BRC: example of specifying custom bin output dir
    # add_subdirectory("${src_subdir}" "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/${src_subdir}")
    add_subdirectory("${src_subdir}")
  endif()
endfunction()


