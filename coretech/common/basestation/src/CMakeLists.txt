# coretech/common/basestation/src/CMakeLists.txt


file(GLOB SOURCE_FILES *.cpp math/*.cpp)

file(GLOB HEADER_FILES 
	*.h 
	math/*.h 
	${CORETECH_COMMON_INCLUDE_DIR}/basestation/*.h
  ${CORETECH_COMMON_INCLUDE_DIR}/basestation/math/*.h
)

add_library(CoreTech_Common_Basestation
	${SOURCE_FILES}
	${HEADER_FILES}
	${CORETECH_COMMON_INCLUDE_FILES}
)

