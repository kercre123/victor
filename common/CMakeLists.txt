# coretech-common/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

include(anki.cmake)

ankiProject(CoreTech_Common)

#set(CORETECH_COMMON_LIBNAME CoreTech_Common_${ANKI_LIBNAME_SUFFIX})
#set(CORETECH_COMMON_LIBNAME_EMBEDDED CoreTech_Common_Embedded_${ANKI_LIBNAME_SUFFIX})

add_subdirectory(src) 
add_subdirectory(test)
add_subdirectory(matlab/mex)
