cmake_minimum_required(VERSION 3.6)
project(unitygenerated-il2cpp)

set(CXX_FLAGS "-Os -std=c++11 -stdlib=libc++")
set(CXX_FLAGS_IL2CPP "-Wno-return-type -Wno-extern-initializer")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -miphoneos-version-min=8.0")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CXX_FLAGS} ${CXX_FLAGS_IL2CPP}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

file(GLOB NATIVE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/Classes/Native/*.cpp")

# message(STATUS "NATIVE_SRC: " ${NATIVE_SRC})

set(INCLUDES
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Classes/Native>
    $<BUILD_INTERFACE:$ENV{ANKI_BUILD_UNITY_HOME}/PlaybackEngines/iOSSupport/il2cpp/libil2cpp/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/bdwgc/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/libil2cpp/include>
)

add_library(unity-generated-armv7 STATIC ${NATIVE_SRC})
target_include_directories(unity-generated-armv7 PRIVATE ${INCLUDES})
set_target_properties(unity-generated-armv7 PROPERTIES COMPILE_FLAGS "-arch armv7")

add_library(unity-generated-arm64 STATIC ${NATIVE_SRC})
target_include_directories(unity-generated-arm64 PRIVATE ${INCLUDES})
set_target_properties(unity-generated-arm64 PROPERTIES COMPILE_FLAGS "-arch arm64")

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/libunity-generated.a
    COMMAND lipo -create libunity-generated-armv7.a libunity-generated-arm64.a -o libunity-generated.a
    DEPENDS unity-generated-armv7 unity-generated-arm64
    COMMENT "Combining libunity-generated armv7,arm64")

add_custom_target(libunity-generated ALL DEPENDS ${CMAKE_BINARY_DIR}/libunity-generated.a)
