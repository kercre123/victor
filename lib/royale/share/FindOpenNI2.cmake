#****************************************************************************
# Copyright (C) 2018 pmdtechnologies ag & Infineon Technologies
#
# THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
# KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
# PARTICULAR PURPOSE.
#
#****************************************************************************

set(OpenNI2_FOUND 0)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # Regular x86
    set(TMP_PROG_PATH $ENV{PROGRAMW6432})
    set(TMP_OPENNI_ENV_LIB $ENV{OPENNI2_LIB})
    set(TMP_OPENNI_ENV_INC $ENV{OPENNI2_INCLUDE})
else()
    # x64
    set(TMP_PROG_PATH $ENV{PROGRAMFILES})
    set(TMP_OPENNI_ENV_LIB $ENV{OPENNI2_LIB64})
    set(TMP_OPENNI_ENV_INC $ENV{OPENNI2_INCLUDE64})
endif()

set(OPENNI2_DEFINITIONS ${PC_OPENNI_CFLAGS_OTHER})
FIND_LIBRARY( OPENNI2_LIBRARY
             NAMES OpenNI2
             HINTS ${PC_OPENNI2_LIBDIR} ${PC_OPENNI2_LIBRARY_DIRS} /usr/lib
             PATHS "${TMP_PROG_PATH}/OpenNI2/Lib${OPENNI2_SUFFIX}" "$ENV{PROGRAMW6432}/OpenNI2" "${TMP_OPENNI_ENV_LIB}"
             PATH_SUFFIXES lib lib64
)
FIND_PATH( OPENNI2_INCLUDE_DIR OpenNI.h
          HINTS ${PC_OPENNI2_INCLUDEDIR} ${PC_OPENNI2_INCLUDE_DIRS} 
                  /usr/include/openni2 /usr/include/ni2
                  PATHS "${TMP_PROG_PATH}/OpenNI2/include" "${TMP_OPENNI_ENV_INC}"
          PATH_SUFFIXES openni2 ni2)

if (OPENNI2_LIBRARY AND OPENNI2_INCLUDE_DIR)
    set(OpenNI2_FOUND 1)
    set(OPENNI2_DEFINITIONS ${PC_OPENNI_CFLAGS_OTHER})
    
    message(STATUS "OpenNI2 found")
    message(STATUS "OpenNI2 library : " ${OPENNI2_LIBRARY})
    message(STATUS "OpenNI2 include dir : " ${OPENNI2_INCLUDE_DIR})
endif()