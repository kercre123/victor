# coretech/planning/basestation/CMakeLists.txt

add_definitions(-DCORETECH_BASESTATION)

add_subdirectory(src)

if(NOT ANKI_IOS_BUILD)
  add_subdirectory(test)
endif()
