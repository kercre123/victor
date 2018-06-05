# Copyright (c) 2018 Anki, Inc.
# All rights reserved.

include(get_all_targets)
include(target_dependencies)

# https://github.com/OpenSourceOrg/licenses

SET(KNOWN_LICENSES
  "AAL" "AFL-3.0" "AGPL-3.0" "APL-1.0" "APSL-2.0" "Apache-1.1" "Apache-2.0" "Artistic-1.0" "Artistic-2.0"
  "BSD-2" "BSD-3" "BSD-4" "BSL-1.0"
  "CATOSL-1.1" "CDDL-1.0" "CECILL-2.1" "CNRI-Python" "CPAL-1.0" "CPL-1.0" "CUA-OPL-1.0" "CVW"
  "ECL-1.0" "ECL-2.0" "EFL-1.0" "EFL-2.0" "EPL-1.0" "EUDatagrid" "EUPL-1.1" "Entessa"
  "Fair" "Frameworx-1.0"
  "GFDL-1.2" "GFDL-1.3" "GPL-1.0" "GPL-2.0" "GPL-3.0"
  "HPND"
  "IPA" "IPL-1.0" "ISC" "ISC:wraptext" "Intel"
  "LGPL-2.0" "LGPL-2.1" "LGPL-3.0" "LPL-1.0" "LPL-1.02" "LPPL-1.3c" "LiLiQ-P-1.1" "LiLiQ-R+" "LiLiQ-R-1.1" 
  "MIT" "MPL-1.0" "MPL-1.1" "MPL-2.0" "MS-PL" "MS-RL" "MirOS" "Motosoto" "Multics"
  "NASA-1.3" "NCSA" "NGPL" "NPOSL-3.0" "NTP" "Naumen" "Nokia"
  "OCLC-2.0" "OFL-1.1" "OGTSL" "OPL-2.1" "OSL-1.0" "OSL-2.1" "OSL-3.0"
  "PHP-3.0" "PostgreSQL" "Python-2.0"
  "QPL-1.0"
  "RPL-1.1" "RPL-1.5" "RPSL-1.0" "RSCPL"
  "SISSL" "SPL-1.0" "Simple-2.0" "Sleepycat"
  "UPL"
  "VSL-1.0"
  "W3C" "WXwindows" "Watcom-1.0"
  "Xnet"
  "ZPL-2.0" "Zlib"

  # additional licenses referenced by Anki documentation but not part of OpenSourceOrg/licenses
  "Anki-Inc." "Apache-1.0" "CC0" "CCBY" "CCBY-SA" "SSL" "WTFPL" "Unlicense" "Proprietary" "Public Domain" "SAAS"
)

# https://ankiinc.atlassian.net/wiki/spaces/ET/pages/380305449/Open+Source+License+Go+Caution+Stop+List+Distributed+Software

set(GO_LICENSES
  "Anki-Inc."
  "Proprietary"

  "Artistic-2.0"
  "Apache-1.1" "Apache-2.0"
  "BSD-2" "BSD-3" "BSD-4" "BSL-1.0"
  "CC0" "CCBY"
  "MIT"
  "PHP-3.0"
  "Public Domain"
  "Python-2.0"
  "WTFPL"
  "Zlib"
  "Unlicense"
)

set(CAUTION_LICENSES
  "Apache-1.0"
  "EPL-1.0"
  "GPL-2.0"
  "LGPL-2.1"
  "MPL-1.1" "MPL-2.0"
  "CDDL-1.0" "CPL-1.0"
  "IPL-1.0"
)

set(STOP_LICENSES
  "GPL-3.0"
  "LGPL-3.0"
  "Sleepycat"
  "CCBY-SA"
  "SAAS"
)

# all messages to cmake output, do not cause a build break
set(LICENSE_CAUTION_MODE STATUS)
set(LICENSE_STOP_MODE STATUS)

function(anki_build_target_license target)
  foreach(arg IN LISTS ARGN)
    # shortcut for Anki, Inc.

    if(arg STREQUAL "ANKI")
      set(arg "Anki-Inc.,${CMAKE_SOURCE_DIR}/LICENSE")
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
      message(STATUS "WARNING: missing path to license file for ${license} on ${target} target")
      return()

    else()
      # missing license, missing file
      message(STATUS "WARNING: missing license information for ${target} target")
      return()
    endif()

    # check against known, caution and stop lists

    list(FIND KNOWN_LICENSES ${license} found)
    if(found EQUAL -1)
      message(STATUS "ERROR: unrecognised license ${license} for ${target} target")
      return()
    endif()

    list(FIND CAUTION_LICENSES ${license} found)
    if(found GREATER_EQUAL 0)
      message(${LICENSE_CAUTION_MODE} "CAUTION: license ${license} for ${target} target needs approval")
      return()
    endif()

    list(FIND STOP_LICENSES ${license} found)
    if(found GREATER_EQUAL 0)
      message(${LICENSE_STOP_MODE} "STOP: license ${license} for ${target} target needs approval")
      return()
    endif()

    # output to binary directory
    if(IS_ABSOLUTE ${file})
      file(COPY ${file} DESTINATION ${CMAKE_BINARY_DIR}/licences/${target}-${license})
    else()
      message(FATAL_ERROR "${target} target is using a relative path, ${file}, for it's ${license} license")
      return()
    endif()
  endforeach()

  # set_property(TARGET ${target} APPEND PROPERTY APPROVED_LICENSE 1)
endfunction()

function(check_licenses)

  # iterate through all targets looking for executables and shared libraries
  get_all_targets(all_targets)

  set(all_executables)
  foreach(target ${all_targets})
    get_property(type TARGET ${target} PROPERTY TYPE)
    if(${type} STREQUAL "EXECUTABLE" OR ${type} STREQUAL "SHARED_LIBRARY")
      list(APPEND all_executables ${target})
    endif()
  endforeach()

  # take the list of executables and shared libraries and recursively add all dependencies
  set(all_dependencies "${all_executables}")
  foreach(target ${all_executables})
    list(APPEND all_dependencies ${deps})
  endforeach()
  list(LENGTH all_dependencies count)
  if(count)
    list(REMOVE_DUPLICATES all_dependencies)
  endif()

  # temporarily remove audio targets until this is in master as audio shares cmake files
  list(REMOVE_ITEM all_dependencies zipreader hijack_audio wave_portal ak_alsa_sink audio_engine audio_multiplexer_engine audio_multiplexer_robot)

  # check all dependencies have license set
  foreach(target ${all_dependencies})
    if(TARGET ${target})
      get_property(type TARGET ${target} PROPERTY TYPE)
      if(${type} STREQUAL "UTILITY")
        # non-source code target, maybe GO

      elseif(${type} STREQUAL "INTERFACE_LIBRARY")
        # non-source code target

      else()
        if(0)
          # guess what the license is to bootstrap license settings
          guess_license_for_target(${target} licenses)
          message("${target} ${license}")
        endif()

        get_property(prop TARGET ${target} PROPERTY APPROVED_LICENSE)
        if(NOT prop)
          message(STATUS "WARNING: licensing information missing or is not approved for ${target} target")
        endif()

      endif()
    else()
      message("### NON TARGET DEPENDENCY ${target}")
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
    endif()

    if(contents MATCHES "redistributions of source code must retain the above copyright" AND
           contents MATCHES "redistributions in binary form must reproduce the above copyright" AND
           contents MATCHES "endorse or promote products")
      list(APPEND licenses "BSD-3")
    endif()

    if(contents MATCHES "redistributions of source code must retain the above copyright" AND
           contents MATCHES "redistributions in binary form must reproduce the above copyright")
      list(APPEND licenses "BSD-2")
    endif()

    if(contents MATCHES "cc0 public domain")
      list(APPEND licenses "CC0")
    endif()

    if(contents MATCHES "gnu " AND
           contents MATCHES "license version 2")
      list(APPEND licenses "GPL-2.0")
    elseif(contents MATCHES "gnu " AND
           contents MATCHES "license")
      list(APPEND licenses "GPL")
    endif()

    if(contents MATCHES "audiokinetic inc")
      list(APPEND licenses "Copyright (c) 2006 Audiokinetic Inc")
    endif()

    if(contents MATCHES "signal essence")
      list(APPEND licenses "Copyright 2017 Signal Essence")
    endif()

    if(contents MATCHES "sergey lyubka")
      list(APPEND licenses "MIT")
    endif()

    if(contents MATCHES "luke benstead")
      list(APPEND licenses "modified BSD")
    endif()
  
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

    guess_license_for_file(${src} addtional_licenses)
    list(APPEND licenses ${addtional_licenses})

    # check same directory for license files
    get_filename_component(dir ${src} DIRECTORY)
    foreach(license_file LICENSE.md LICENSE COPYING)
      if(EXISTS "${dir}/${license_file}")
        guess_license_for_file(${dir}/${license_file} addtional_licenses)
        list(APPEND licenses ${addtional_licenses})
      endif()
    endforeach()

    # check parent directory for license files
    get_filename_component(dir ${dir} DIRECTORY)
    foreach(license_file LICENSE.md LICENSE COPYING)
      if(EXISTS "${dir}/${license_file}")
        guess_license_for_file(${dir}/${license_file} addtional_licenses)
        list(APPEND licenses ${addtional_licenses})
      endif()
    endforeach()
  endforeach()

  list(LENGTH licenses count)
  if(count)
      list(REMOVE_DUPLICATES licenses)
  endif()

  set(${licenses_result} "${licenses}" PARENT_SCOPE)
endfunction()
