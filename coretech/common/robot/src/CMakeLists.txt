# coretech/common/robot/src/CMakeLists.txt

file(GLOB SOURCE_FILES *.c *.cpp)

file(GLOB HEADER_FILES 
	*.h 
	${CORETECH_COMMON_INCLUDE_DIR}/robot/*.h
)

add_library(CoreTech_Common_Robot
	${SOURCE_FILES} 
	${HEADER_FILES}
	${CORETECH_COMMON_INCLUDE_FILES}
)
