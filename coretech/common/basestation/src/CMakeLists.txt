# coretech/common/basestation/src/CMakeLists.txt

if(NOT WIN32)
  file(GLOB SOURCE_FILES *.cpp math/*.cpp utils/*.cpp ../../shared/src/*.cpp)

  file(GLOB HEADER_FILES 
    *.h 
    math/*.h 
    ../../shared/src/*.h
    ${CORETECH_COMMON_INCLUDE_DIR}/basestation/*.h
    ${CORETECH_COMMON_INCLUDE_DIR}/basestation/math/*.h
    ${CORETECH_COMMON_INCLUDE_DIR}/shared/*.h
  )

  add_library(CoreTech_Common_Basestation
    ${SOURCE_FILES}
    ${HEADER_FILES}
    ${CORETECH_COMMON_INCLUDE_FILES}
  )
endif()
