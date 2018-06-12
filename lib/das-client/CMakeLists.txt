project(das_client)

include(anki_build_cxx)
include(anki_build_strip)

option(USE_DAS "Enable DAS logs" OFF)

set(SRCS
  src/DAS.cpp
  src/dasAppender.cpp
  src/dasFilter.cpp
  src/dasGameLogAppender.cpp
  src/dasInit.cpp
  src/dasLogFileAppender.cpp
  src/fileUtils.cpp
  src/stringUtils.cpp
  src/stringUtils.h
  src/taskExecutor.cpp
  src/taskExecutor.h
)

set(PLATFORM_SRCS "")
set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")

if (VICOS)

  message(STATUS "DAS Platform: VICOS")

  set(PLATFORM_SRCS
    vicos/dasLogMacros.h
    vicos/dasLocalAppenderImpl.h
    vicos/dasLocalAppenderImpl.cpp
    vicos/dasLocalAppenderFactory.cpp
    vicos/dasPostToServer.cpp
  )

  set(PLATFORM_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/vicos)
  set(PLATFORM_LIBS log curl z)

elseif (ANDROID)

  message(STATUS "DAS Platform: ANDROID" )

  set(PLATFORM_SRCS
    android/DASNativeLib/jni/dasJni.cpp
    android/DASNativeLib/jni/dasLocalAppenderFactory.cpp
    android/DASNativeLib/jni/dasLocalAppenderImpl.cpp
    android/DASNativeLib/jni/dasLocalAppenderImpl.h
    android/DASNativeLib/jni/dasLogMacros.h
    android/DASNativeLib/jni/dasPlatform_android.cpp
  )
  set(PLATFORM_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/android/DASNativeLib/jni)
  set(PLATFORM_LIBS android log)

  # needed to workaround localeconv issue with c++_shared on android
  target_compile_definitions(DAS PRIVATE -DNO_LOCALE_SUPPORT=1)

elseif (MACOSX)

  message(STATUS "DAS Platform: UNIX")

  set(PLATFORM_SRCS
    unix/dasLocalAppenderFactory.cpp
    unix/dasLocalAppenderImpl.cpp
    unix/dasLocalAppenderImpl.h
    unix/dasLogMacros.h
    unix/dasPostToServer.cpp
  )
  set(PLATFORM_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/unix)

endif()

add_library(DAS
  ${SRCS}
  ${PLATFORM_SRCS}
)

target_compile_options(DAS
  PRIVATE
  ${ANKI_BUILD_CXX_COMPILE_OPTIONS}
)

target_compile_definitions(DAS
  PUBLIC
  USE_DAS=$<BOOL:${USE_DAS}>
  PRIVATE
  ${ANKI_BUILD_CXX_COMPILE_DEFINITIONS}
)

target_include_directories(DAS
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/DAS>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${PLATFORM_INCLUDES}>
)

target_link_libraries(DAS
  ${PLATFORM_LIBS}
  jsoncpp
)

anki_build_target_license(DAS "ANKI")

anki_build_strip(TARGET DAS)
