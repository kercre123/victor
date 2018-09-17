# coretech/CMakeLists.txt

cmake_minimum_required(VERSION 3.6)
project(cti)

include(import)

include(opencv)

add_subdirectory(common/clad_src/clad)
add_subdirectory(vision/clad_src/)

set(CTI_GENERATED_LIBS
    cti_common_clad
    cti_vision_clad
)

import(cti_common common)
import(cti_messaging messaging)
import(cti_neuralnets neuralnets)
import(cti_planning planning)
import(cti_vision vision)

if (MACOSX)
  enable_testing()
endif(MACOSX)
