#
# As of cmake 3.9.6, the ninja generator assumes that ".def" files are module definition
# files. If a ".def" file appears as an input file, cmake will generate a (bogus) build rule
# to create it.
#
# Workaround for this behavior is to set source file property HEADER_FILE_ONLY on 
# each ".def" file.  This informs cmake that the file should be treated as a regular 
# header file, not a module definition file.
# 
# See also 
#   https://stackoverflow.com/questions/34659795/cmake-how-to-add-a-def-file-which-is-not-a-module-definition-file
#
# Resolves cryptic build errors:
#   /Users/build/.anki/cmake/dist/3.9.6/CMake.app/Contents/bin/cmake -E __create_def ...
#   CMake Error: cmake version 3.9.6
#   Usage: cmake -E <command> [arguments...]
#

macro(anki_build_header_file_only_list)
  foreach(header ${ARGN})
    get_filename_component(ext ${header} EXT)
    if (ext STREQUAL ".def")
      set_source_files_properties(${header} PROPERTIES HEADER_FILE_ONLY TRUE)
    endif()
  endforeach()
endmacro()

