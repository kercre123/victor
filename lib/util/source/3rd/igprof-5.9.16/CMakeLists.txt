PROJECT(IGPROF C CXX)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
FIND_PACKAGE(Threads)

#Find PCRE library
IF(PCRE_INCLUDE_DIR AND PCRE_LIBRARIES)
  SET(PCRE_FOUND TRUE)
ELSE(PCRE_INCLUDE_DIR AND PCRE_LIBRARIES)

  FIND_PATH(PCRE_INCLUDE_DIR NAMES pcre.h
            PATHS /usr/include /usr/local/include)

  FIND_LIBRARY(PCRE_LIBRARY NAMES pcre
               PATHS /usr/lib /usr/local/lib)


  IF(PCRE_INCLUDE_DIR AND PCRE_LIBRARY)
    SET(PCRE_LIBRARIES ${PCRE_LIBRARY})
    SET(PCRE_FOUND TRUE)
  ELSE(PCRE_INCLUDE_DIR AND PCRE_LIBRARY)
    SET(PCRE_FOUND FALSE)
  ENDIF(PCRE_INCLUDE_DIR AND PCRE_LIBRARY)

ENDIF(PCRE_INCLUDE_DIR AND PCRE_LIBRARIES)

# ARM Check if arm CCNT instruction is allowed
IF(${CMAKE_SYSTEM_PROCESSOR} MATCHES arm OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES aarch64)
  TRY_RUN(RUN_RESULT_VAR COMPILE_RESULT_VAR
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/test_user_ccnt.c)
  IF(${RUN_RESULT_VAR})
    SET(USER_CCNT TRUE CACHE BOOL "Whether CCNT instruction is available")
    MESSAGE("${RUN_RESULT_VAR}")
  ELSE()
    SET(USER_CCNT FALSE CACHE BOOL "Whether CCNT instruction is available") 
  ENDIF()
ENDIF()

# Find PAPI library
IF(PAPI_INCLUDE_DIR AND PAPI_LIBRARY)
  SET(PAPI_FOUND TRUE)
ELSE(PAPI_INCLUDE_DIR AND PAPI_LIBRARY)
  FIND_PATH(PAPI_INCLUDE_DIR NAMES papi.h
            PATHS /usr/include /usr/local/include)
  FIND_LIBRARY(PAPI_LIBRARY NAMES papi
               PATHS /usr/lib /usr/local/lib)
  IF(PAPI_INCLUDE_DIR AND PAPI_LIBRARY)
    SET(PAPI_FOUND TRUE)
  ELSE(PAPI_INCLUDE_DIR AND PAPI_LIBRARY)
    SET(PAPI_FOUND FALSE)
  ENDIF(PAPI_INCLUDE_DIR AND PAPI_LIBRARY)
ENDIF(PAPI_INCLUDE_DIR AND PAPI_LIBRARY)
IF(PAPI_FOUND)
  SET(IGPROF_LIBS ${IGPROF_LIBS} ${PAPI_LIBRARY})
ENDIF(PAPI_FOUND)

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/config.h.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h)

# Options.
OPTION(IGPROF_BUILD_TESTS "Build tests." OFF)

# Default to "release with debug info", i.e. "-O2 -g".
IF(NOT CMAKE_CONFIGURATION_TYPE AND NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Build type: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF()

# Flags we need.
ADD_DEFINITIONS(-D__STDC_FORMAT_MACROS)
ADD_DEFINITIONS(-D__STDC_LIMIT_MACROS)
IF(${CMAKE_SYSTEM_NAME} MATCHES Linux)
  ADD_DEFINITIONS(-D_GNU_SOURCE)
  SET(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS_INIT} -Wl,-z,nodefs
      CACHE STRING "Flags used by the linker during the creation of dll's.")
ENDIF()

# Use ANSI mode for C and stricter compile options with GCC.
SET(CMAKE_C_FLAGS "${CMAKE_ANSI_FLAGS} ${CMAKE_C_FLAGS}")
SET(CMAKE_REQUIRED_FLAGS ${CMAKE_ANSI_FLAGS})
IF(CMAKE_COMPILER_IS_GNUCC)
  ADD_DEFINITIONS(-ansi -pedantic -W -Wall -Wno-long-long -Werror)
ENDIF()

IF(UNIX)
  SET(IGPROF_LIBS ${IGPROF_LIBS} ${CMAKE_DL_LIBS})
ENDIF()

IF(${CMAKE_SYSTEM_PROCESSOR} MATCHES 64 OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES arm)
  FIND_PATH(UNWIND_INCLUDE_DIR libunwind.h)
  FIND_LIBRARY(UNWIND_LIBRARY NAMES unwind)
ENDIF()

# Build web page
FIND_PATH(PROTOVIS_ROOT protovis-r3.2.js)
FIND_PATH(JQUERY_ROOT jquery-1.4.4.min.js)
FIND_PATH(JQUERY_BA_HASHCHANGE_ROOT jquery.ba-hashchange.min.js)
IF(JQUERY_BA_HASHCHANGE_ROOT AND JQUERY_ROOT AND PROTOVIS_ROOT)
ADD_CUSTOM_TARGET(igprof-book ALL
                  COMMAND src/igprof-book-build
                            --js-path ${JQUERY_ROOT}:${PROTOVIS_ROOT}:${JQUERY_BA_HASHCHANGE_ROOT}
                            src/igprof-book.html
                            --driver src/igprof-book.in > igprof-book
                  DEPENDS src/igprof-book.html src/igprof-book src/igprof-book.js)
INSTALL(FILES igprof-book
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
ENDIF()

# Build targets.
IF(PCRE_FOUND)
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR} ${UNWIND_INCLUDE_DIR}
                      ${PCRE_INCLUDE_DIR})
ELSE()
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR} ${UNWIND_INCLUDE_DIR})
ENDIF()
IF(PAPI_FOUND)
  # Treat the directory containing papi.h as a system include directory in
  # order to suppress the "comma at end of enumerator list" error. The PAPI
  # header file does not conform to ANSI C.
  INCLUDE_DIRECTORIES(SYSTEM ${PAPI_INCLUDE_DIR})
ENDIF()
ADD_LIBRARY(igprof SHARED
            src/hook.cc
            src/buffer.cc
            src/sym-cache.cc
            src/walk-syms.cc
            src/profile.cc
            src/profile-fd.cc
            src/profile-mem.cc
            src/profile-empty.cc
            src/profile-perf.cc
            src/profile-trace.cc
            src/profile-calls.cc
            src/profile-finstrument.cc
            src/profile-energy.cc
            src/trace.cc
            src/trace-mem.cc
            src/trace-mmap.cc
            src/trace-throw.cc)
#SET_TARGET_PROPERTIES(igprof PROPERTIES LINK_FLAGS -Wl,-z,nodefs)

TARGET_LINK_LIBRARIES(igprof ${UNWIND_LIBRARY} ${IGPROF_LIBS} ${CMAKE_THREAD_LIBS_INIT})
ADD_EXECUTABLE(igprof-analyse src/analyse.cc)
IF(PCRE_FOUND)
  TARGET_LINK_LIBRARIES(igprof-analyse ${CMAKE_DL_LIBS} ${PCRE_LIBRARIES})
ELSE()
  TARGET_LINK_LIBRARIES(igprof-analyse ${CMAKE_DL_LIBS})
ENDIF()
INSTALL(TARGETS igprof igprof-analyse
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)
INSTALL(FILES
        src/igprof-analyse-old
        src/igprof-func
        src/igprof-navigator
        src/igprof-navigator-summary
        src/igprof-segment
        src/igprof-populator
        src/igprof-symbol-sizes
        src/igprof
        src/igpython-analyse
        src/igtrace
        src/igtrace-mmap-analysis
        src/igtrace-mmap-summary
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
INSTALL(FILES src/sym-resolve.h DESTINATION include/igprof)

# Tests.
IF(IGPROF_BUILD_TESTS)
  # FIXME: Build libraries from sources in test/*.cc.
  # FIXME: Run igprof-regression-tests
ENDIF()
