project(das_client)

set(SRCS
  src/DAS.cpp
  src/dasAppender.cpp
  src/dasFilter.cpp
  src/dasGameLogAppender.cpp
  src/dasInit.cpp
  src/dasLogFileAppender.cpp
  src/fileUtils.cpp
  src/jsoncpp.cpp
  src/stringUtils.cpp
  src/stringUtils.h
  src/taskExecutor.cpp
  src/taskExecutor.h
)

set(PLATFORM_SRCS "")
if (ANDROID)
  set(PLATFORM_SRCS
    android/DASNativeLib/jni/dasJni.cpp
    android/DASNativeLib/jni/dasLocalAppenderFactory.cpp
    android/DASNativeLib/jni/dasLocalAppenderImpl.cpp
    android/DASNativeLib/jni/dasLocalAppenderImpl.h
    android/DASNativeLib/jni/dasLogMacros.h
    android/DASNativeLib/jni/dasPlatform_android.cpp
  )
elseif (MACOSX OR VICOS)
  set(PLATFORM_SRCS
    unix/dasLocalAppenderFactory.cpp
    unix/dasLocalAppenderImpl.cpp
    unix/dasLocalAppenderImpl.h
    unix/dasLogMacros.h
    unix/dasPostToServer.cpp
  )
endif()

set(PLATFORM_LIBS "")
if (ANDROID)
  set(PLATFORM_LIBS android log)
elseif(VICOS)
  set(PLATFORM_LIBS log)
endif()

add_library(DAS
  ${SRCS}
  ${PLATFORM_SRCS}
)
include(android_strip)
android_strip(TARGET DAS)

set(PLATFORM_INCLUDES "")

if (ANDROID)
  # needed to workaround localeconv issue with c++_shared on android
  target_compile_definitions(DAS PRIVATE -DNO_LOCALE_SUPPORT=1)
  set(PLATFORM_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/android/DASNativeLib/jni)
elseif (MACOSX OR VICOS)
  set(PLATFORM_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/unix)
endif()

target_include_directories(DAS PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/DAS>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${PLATFORM_INCLUDES}>
)

target_link_libraries(DAS
  ${PLATFORM_LIBS}
)
