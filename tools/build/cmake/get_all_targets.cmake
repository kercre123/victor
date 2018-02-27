# recursively iterate through all target source directories collecting targets
function(get_all_targets TARGET)
  get_directory_property(result BUILDSYSTEM_TARGETS)

  get_directory_property(subdirs SUBDIRECTORIES)
  while(subdirs)
    list(GET subdirs 0 dir)
    list(REMOVE_AT subdirs 0)
    get_directory_property(dirs DIRECTORY ${dir} SUBDIRECTORIES)
    list(APPEND subdirs ${dirs})

    get_directory_property(targets DIRECTORY ${dir} BUILDSYSTEM_TARGETS)
    list(APPEND result ${targets})
  endwhile()

  set(${TARGET} ${result} PARENT_SCOPE)
endfunction()
