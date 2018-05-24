cmake_minimum_required(VERSION 3.6)

project(test)

add_subdirectory(animProcess)
add_subdirectory(engine)

if (VICOS)
  add_subdirectory(switchboard)
endif(VICOS)

if (MACOSX)
  enable_testing()
endif(MACOSX)
