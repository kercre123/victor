# coretech-vision/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

include(../coretech-common/anki.cmake)

ankiProject(CoreTech_Vision)

#set(CORETECH_VISION_LIBNAME CoreTech_Vision_${ANKI_LIBNAME_SUFFIX})
#set(CORETECH_VISION_LIBNAME_EMBEDDED CoreTech_Vision_Embedded_${ANKI_LIBNAME_SUFFIX})

add_subdirectory(src)
add_subdirectory(test) 
add_subdirectory(matlab/mex)
add_subdirectory(matlab/CameraCapture)