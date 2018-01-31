#
# Anki provides gtest as a MacOS framework.  Locate
# gtest.framework by probing cmake module path until 
# we find a match.
#
if (NOT TARGET gtest)
  if (NOT GTEST_FRAMEWORK)
    foreach (DIR ${CMAKE_MODULE_PATH})
      if (EXISTS ${DIR}/../libs/framework/gtest.framework)
        set(GTEST_FRAMEWORK ${DIR}/../libs/framework/gtest.framework)
        break()
      endif()
    endforeach()
  endif()

  #
  # If we didn't find the framework, fail with appropriate error
  #
  if (NOT GTEST_FRAMEWORK)
    message(FATAL_ERROR "Unable to find gtest.framework in CMAKE_MODULE_PATH")
    return()
  endif()

  #
  # Declare an interface target for this framework
  #
  add_library(gtest INTERFACE IMPORTED)

  #
  # Set interface properties to be inherited by downstream targets
  #
  set_target_properties(gtest 
    PROPERTIES
    INTERFACE_COMPILE_OPTIONS -Wno-undef
    INTERFACE_INCLUDE_DIRECTORIES ${GTEST_FRAMEWORK}
    INTERFACE_LINK_LIBRARIES ${GTEST_FRAMEWORK}
  )
endif()