# coretech/common/basestation/src/CMakeLists.txt


file(GLOB SOURCE_FILES *.cpp math/*.cpp)
file(GLOB HEADER_FILES *.h math/*.h)

add_library(CoreTech_Common_Basestation
	${SOURCE_FILES}
	${HEADER_FILES}
)

