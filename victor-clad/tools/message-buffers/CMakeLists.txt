project(libclad)

add_library(clad STATIC
    support/cpp/source/SafeMessageBuffer.cpp)

target_include_directories(clad PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/support/cpp/include>)

anki_build_target_license(clad "ANKI")
