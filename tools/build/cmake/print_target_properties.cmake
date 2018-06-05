# Get all propreties that cmake supports
execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)

# Convert command output into a CMake list
STRING(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
STRING(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")

function(print_properties)
  message("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
endfunction()

function(print_target_properties tgt)
  if(NOT TARGET ${tgt})
    message("There is no target named '${tgt}'")
    return()
  endif()

  foreach (prop ${CMAKE_PROPERTY_LIST})
    string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" prop_with_config ${prop})

    # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
    if(prop_with_config STREQUAL "LOCATION" OR prop_with_config MATCHES "^LOCATION_" OR prop_with_config MATCHES "_LOCATION$")
      continue()
    endif()

    get_property(propval TARGET ${tgt} PROPERTY ${prop_with_config} SET)
    if (propval)
      get_target_property(propval ${tgt} ${prop_with_config})
      message ("${tgt} ${prop} = ${propval}")
    endif()
  endforeach()
endfunction()

function(print_directory_properties dir)
  foreach (prop ${CMAKE_PROPERTY_LIST})
    string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" prop_with_config ${prop})

    get_directory_property(propval DIRECTORY ${dir} ${prop_with_config})
    if (propval)
      message ("${dir} ${prop_with_config} = ${propval}")
    endif()
  endforeach()
endfunction()
