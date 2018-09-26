# Copyright (c) 2018 Anki, Inc.
# All rights reserved.

include(get_all_targets)
include(target_dependencies)

if(NOT MACOSX)
option(ENFORCE_LICENSES "Fail a build on any license irregularities." OFF)
endif()

if(ENFORCE_LICENSES)
  set(MESSAGE_STATUS FATAL_ERROR)
else()
  set(MESSAGE_STATUS STATUS)
endif()

file(STRINGS ${CMAKE_SOURCE_DIR}/docs/development/licenses.md license_file ENCODING UTF-8)
list(REMOVE_AT license_file 0)
foreach(line ${license_file})
    string(SUBSTRING "${line}" 0 1 firstchar)
    if(NOT firstchar STREQUAL "|")
      continue()
    endif()

    string(REPLACE "|" ";" columns "${line}")

    list(LENGTH columns columns_count)
    if(NOT columns_count EQUAL 6)
      message(FATAL_ERROR "Malformed license table, with ${columns_count} columns instead of 6 for '${line}'")
    endif()

    list(GET columns 1 license)
    list(GET columns 2 approval)

    string(STRIP "${license}" license)
    string(STRIP "${approval}" approval)

    if(approval STREQUAL "Go")
        list(APPEND GO_LICENSES ${license})
    elseif(approval STREQUAL "Stop")
        list(APPEND STOP_LICENSES ${license})
    elseif(approval STREQUAL "Caution")
        list(APPEND CAUTION_LICENSES ${license})
    elseif(approval STREQUAL "Reviewing")
        list(APPEND REVIEWING_LICENSES ${license})
    endif()
    list(APPEND ALL_LICENSES ${license})
endforeach()

function(anki_build_target_license target)
  set_property(TARGET ${target} APPEND PROPERTY APPROVED_LICENSE 0)

  foreach(arg IN LISTS ARGN)

    # Anki and Commercial licenses need no license file

    if(arg STREQUAL "ANKI")
      continue()
    endif()

    # split string separated by comma, e.g. "license,file"

    string(REPLACE "," ";" fields "${arg}")
    list(LENGTH fields fields_count)
    if(fields_count EQUAL 2)
      # license,file
      list(GET fields 0 license)
      list(GET fields 1 file)

    elseif(fields_count EQUAL 1)
      # license, missing file
      list(GET fields 0 license)
      unset(file)

    else()
      # missing license, missing file
      message(${MESSAGE_STATUS} "WARNING: missing license information for ${target} target")
      return()
    endif()

    # don't publish commercial licenses

    if(license STREQUAL "Commercial")
      continue()
    endif()

    # check against known, caution and stop lists

    string(REPLACE "-like" "" license "${license}") # ignore -like licenses defer to base license

    list(FIND ALL_LICENSES ${license} found)
    if(found EQUAL -1)
      message(${MESSAGE_STATUS} "ERROR: unrecognised license ${license} for ${target} target")

      # override previous licensing information
      set_property(TARGET ${target} PROPERTY APPROVED_LICENSE 0)

      return()
    endif()

    list(FIND REVIEWING_LICENSES ${license} found)
    if(found GREATER_EQUAL 0)
      message(${MESSAGE_STATUS} "WARNING: license ${license} for ${target} target is under review")

      # override previous licensing information
      set_property(TARGET ${target} PROPERTY APPROVED_LICENSE 0)

      return()
    endif()

    list(FIND CAUTION_LICENSES ${license} found)
    if(found GREATER_EQUAL 0)
      message(${MESSAGE_STATUS} "CAUTION: license ${license} for ${target} target needs approval")

      # override previous licensing information
      set_property(TARGET ${target} PROPERTY APPROVED_LICENSE 0)

      return()
    endif()

    list(FIND STOP_LICENSES ${license} found)
    if(found GREATER_EQUAL 0)
      message(${MESSAGE_STATUS} "STOP: license ${license} for ${target} target needs approval")

      # override previous licensing information
      set_property(TARGET ${target} PROPERTY APPROVED_LICENSE 0)

      return()
    endif()

    if(file)
      # output to binary directory
      if(IS_ABSOLUTE ${file})
        if (file MATCHES "/licenses/")
          # licenses are of the form: /licences/<name of library>.license

          get_filename_component(dir ${file} NAME_WE)

        elseif (file MATCHES "/3rd/")
          # third party targets are of the form: /lib/util/source/3rd/<name of library/<arbitrary path>/<aribtrary license>

          get_filename_component(dirs ${file} DIRECTORY)
          string(REPLACE "/" ";" dirs "${dirs}")
          list(GET dirs 0 dir)
          list(REMOVE_AT dirs 0)
          while(NOT dir STREQUAL "3rd")
            list(GET dirs 0 dir)
            list(REMOVE_AT dirs 0)
          endwhile()
          list(GET dirs 0 dir)
          list(REMOVE_AT dirs 0)

        elseif (file MATCHES "/go/")
          # Go targets are of the form /cloud/go/src/github.com/<arbitrary path>/<name of library>/LICENSE

          get_filename_component(dir ${file} DIRECTORY)
          string(REPLACE "/" ";" dir "${dir}")
          list(REVERSE dir)
          list(GET dir 0 dir)
        else()
          get_filename_component(dir ${file} NAME)

        endif()

        get_filename_component(filename ${file} NAME)
        if(NOT EXISTS "${CMAKE_BINARY_DIR}/licences/${dir}-${license}/${filename}.txt")
          # copy license to folder
          file(COPY ${file}
               DESTINATION ${CMAKE_BINARY_DIR}/licences/${dir}-${license})

          if (NOT ${filename} MATCHES ".txt")
            # add .txt if it's missing
            file(RENAME
                 ${CMAKE_BINARY_DIR}/licences/${dir}-${license}/${filename}
                 ${CMAKE_BINARY_DIR}/licences/${dir}-${license}/${filename}.txt)
            set(filename "${filename}.txt")
          endif()
        endif()

      else()
        message(FATAL_ERROR "${target} target is using a relative path, ${file}, for it's ${license} license")
        return()
      endif()
    else()
      message(${MESSAGE_STATUS} "WARNING: missing path to license file for ${license} on ${target} target")
      return()
    endif()
  endforeach()

  set_property(TARGET ${target} PROPERTY APPROVED_LICENSE 1)
endfunction()

function(write_license_html)

  file(GLOB_RECURSE files "${CMAKE_BINARY_DIR}/licences/*.*")

  # TODO: VIC-5668 Add version numbers to licenses report and uploads

  # header

  file(WRITE ${CMAKE_BINARY_DIR}/licences/vectorLicenseReport.html
      "<!DOCTYPE html><html><body>\n"
      "<h1>License Data for Vector Engine 1.0.0</h1><p>\n")

  # create html link to folder/file

  foreach(file ${files})
    if (NOT file MATCHES "vectorLicenseReport.html")
      get_filename_component(filename ${file} NAME)
      get_filename_component(dir ${file} DIRECTORY)
      string(REPLACE "/" ";" dir "${dir}")
      list(REVERSE dir)
      list(GET dir 0 dir)
      file(APPEND ${CMAKE_BINARY_DIR}/licences/vectorLicenseReport.html "<a href=\"${dir}/${filename}\"\>${dir}</a><br/>\n")
    endif()
  endforeach()
  
  # footer

  file(APPEND ${CMAKE_BINARY_DIR}/licences/vectorLicenseReport.html "</p></body></html>")
endfunction()

function(check_licenses)

  # iterate through all targets looking for executables and shared libraries
  get_all_targets(all_targets)

  set(all_executables)
  foreach(target ${all_targets})
    get_property(type TARGET ${target} PROPERTY TYPE)
    if(${type} STREQUAL "EXECUTABLE" OR ${type} STREQUAL "SHARED_LIBRARY")
      list(APPEND all_executables ${target})

    elseif(${type} STREQUAL "UTILITY")
      get_property(anki_out_path_isset TARGET ${target} PROPERTY ANKI_OUT_PATH SET)
      get_property(exclude_from_all_isset TARGET ${target} PROPERTY EXCLUDE_FROM_ALL SET)
      if(anki_out_path_isset AND exclude_from_all_isset)
        get_property(exclude_from_all TARGET ${target} PROPERTY EXCLUDE_FROM_ALL)
        if (NOT exclude_from_all)
          # Go executables, deployed to the rebot, have EXCLUDE_FROM_ALL set to FALSE
          list(APPEND all_executables ${target})
        endif()
      endif()
    endif()
  endforeach()

  # take the list of executables and shared libraries and recursively add all dependencies
  set(all_dependencies "${all_executables}")
  foreach(target ${all_executables})
    target_dependencies(${target} deps)
    list(APPEND all_dependencies ${deps})
  endforeach()
  list(LENGTH all_dependencies count)
  if(count)
    list(REMOVE_DUPLICATES all_dependencies)
  endif()

  # check all dependencies have license set
  foreach(target ${all_dependencies})

    set(system_lib FALSE)
    # split full name and partial name matches to avoid over-matching anything with a m or z
    # and simplify matching .so files with version numbers
    # also handle special case, extra linker options (-wl) on the link line

    foreach(lib asound c curl cutils dl log m z zlib)
      if(${target} STREQUAL ${lib})
        set(system_lib TRUE)
      endif()
    endforeach()

    foreach(lib libcutils libglib libgio libgobject libffi libdl libz libresolv libgmodule libpcre
                Accelerate AppKit AudioToolbox AudioUnit CoreAudio CoreBluetooth CoreFoundation
                OpenCL OpenGL Foundation GLUT Security 
                "-Wl" "-ldl"
                ankiutil                     # hack: because of other hacks
                Controller CppController ode # webots
                opus                         # cloud
                gtest                        # special case, imported library can't have our checks
                babile                       # special case, imported library can't have our checks
                libtensorflow-lite.a         # handled elsewhere
                )
      if(${target} MATCHES ${lib})
        set(system_lib TRUE)
      endif()
    endforeach()

    if(TARGET ${target})
      get_property(type TARGET ${target} PROPERTY TYPE)
      if(${type} STREQUAL "INTERFACE_LIBRARY")
        # non-source code target

      else()
        get_property(isset TARGET ${target} PROPERTY APPROVED_LICENSE)

        if(1)
          if(NOT (isset OR system_lib))
            message(${MESSAGE_STATUS} "WARNING: licensing information missing or not approved for ${target} target")
          endif()

        else()
          # guess what the license is to bootstrap license settings
          guess_license_for_target(${target} licenses)

          if(licenses)
            if(isset)
              message("${target} has approved licenses, guessed ${licenses}")
            else()
              message("${target} is not approved, add one or more of ${licenses}")
            endif()
          else()
            if(isset)
              message("${target} has approved licenses")
            else()
              message("${target} is not approved, did not guess any licenses")
            endif()
          endif()
        endif()


      endif()
    else()
      # linker option is not a known target, i.e. a system library or missing cmake configuration

      if(NOT system_lib)
        message("${target} is not a recognised system lib and does not have cmake configuration including licensing information.")
      endif()

    endif()
  endforeach()
endfunction()

# end-of-license checking code

# Helper function, given the text of a file, determine what license it might be

function(guess_license_for_file filename licenses_result)
  set(licenses)

  # if file is referenced in a project but does not exist it must be generated
  # if it's generated, it can be presumed our license
  if(EXISTS "${filename}")
    file(READ ${filename} contents LIMIT 2048)
    string(REPLACE "'" "" "${contents}" contents)
    string(TOLOWER "${contents}" contents)
  
    if(contents MATCHES "anki")
      list(APPEND licenses "Anki")
    endif()

    if(contents MATCHES "kevin yoon" OR contents MATCHES "jordan rivas" OR
          contents MATCHES "lee crippen" OR contents MATCHES "chapados" OR
          contents MATCHES "andrew stein" OR contents MATCHES "brad neuman" OR
          contents MATCHES "matt michini" OR contents MATCHES "jarrod hatfield" OR
          contents MATCHES "al chaussee" OR contents MATCHES "greg nagel" OR
          contents MATCHES "paul aluri")
      list(APPEND licenses "Anki")
    endif()

    if(contents MATCHES "frank thilo")
      list(APPEND licenses "LGPL-2.1")
      list(APPEND licenses "MPL-2.0")
    endif()

    if(contents MATCHES "mit license")
      list(APPEND licenses "MIT")

    elseif(contents MATCHES "permission is hereby granted, free of charge, to any person" AND
           contents MATCHES "the above copyright notice and this permission notice shall" AND
           contents MATCHES "the software is provided \"as is\", without warranty of any kind")
      list(APPEND licenses "MIT")
    endif()

    if(contents MATCHES "apache license")
      if(contents MATCHES "version 2.0")
        list(APPEND licenses "Apache-2.0")
      endif()
    endif()

    if(contents MATCHES "redistributions of source code must retain the" AND
       contents MATCHES "redistributions in binary form must reproduce" AND
       contents MATCHES "endorse or promote products")
      list(APPEND licenses "BSD-3")

    elseif(contents MATCHES "redistributions of source code must retain the" AND
           contents MATCHES "redistributions in binary form must reproduce")
      list(APPEND licenses "BSD-2")
    endif()

    if(contents MATCHES "cc0 public domain")
      list(APPEND licenses "CC0")
    endif()

    if(contents MATCHES "RSA Data Security")
      list(APPEND licenses "RSA")
    endif()

    if(contents MATCHES "gnu general public license" AND
       contents MATCHES "version 2")
      list(APPEND licenses "GPL-2.0")

    elseif(contents MATCHES "gnu general public license")
      list(APPEND licenses "GPL")
    endif()

    if(contents MATCHES "audiokinetic inc")
      list(APPEND licenses "Copyright (c) 2006 Audiokinetic Inc")
    endif()

    if(contents MATCHES "signal essence")
      list(APPEND licenses "Commercial")
    endif()

    if(contents MATCHES "sergey lyubka")
      list(APPEND licenses "MIT")
    endif()

    if(contents MATCHES "luke benstead")
      list(APPEND licenses "modified BSD")
    endif()

    if(contents MATCHES "mozilla public license")
      if(contents MATCHES "version 2.0")
        list(APPEND licenses "MPL-2.0")
      endif()
    endif()

    if(contents MATCHES "permission to use, copy, modify, and/or distribute" AND
       contents MATCHES "the software is provided \"as is\" and the author disclaims all warranties")
      list(APPEND licenses "ISC")
    endif()

    foreach(license ${licenses})
      if(license MATCHES "GPL")
        message("### ${filename} matches against a GPL license")
      endif()
    endforeach()

  endif()

  set(${licenses_result} "${licenses}" PARENT_SCOPE)

endfunction()

# Helper function, given a target, for each source file determine what license it might be

function(guess_license_for_target target licenses_result)
  set(licenses)

  get_property(sources TARGET ${target} PROPERTY SOURCES)
  get_property(source_dir TARGET ${target} PROPERTY SOURCE_DIR)

  foreach(src ${sources})
    if(NOT IS_ABSOLUTE ${src})
      set(src "${source_dir}/${src}")
    endif()

    guess_license_for_file(${src} additional_licenses)
    list(APPEND licenses ${additional_licenses})

    # check same directory for license files
    get_filename_component(dir ${src} DIRECTORY)
    foreach(license_file LICENSE.md LICENSE COPYING)
      if(EXISTS "${dir}/${license_file}")
        guess_license_for_file(${dir}/${license_file} additional_licenses)
        if(additional_licenses)
          list(APPEND licenses "${additional_licenses},${dir}/${license_file}")
        else()
          list(APPEND licenses "UNKNOWN,${dir}/${license_file}")
        endif()
      endif()
    endforeach()

    # check parent directory for license files
    get_filename_component(dir ${dir} DIRECTORY)
    foreach(license_file LICENSE.md LICENSE COPYING)
      if(EXISTS "${dir}/${license_file}")
        guess_license_for_file(${dir}/${license_file} additional_licenses)
        list(APPEND licenses ${additional_licenses})
      endif()
    endforeach()
  endforeach()

  list(LENGTH licenses count)
  if(count)
      list(REMOVE_DUPLICATES licenses)
  endif()

  set(${licenses_result} "${licenses}" PARENT_SCOPE)
endfunction()
