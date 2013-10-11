# coretech/common/robot/src/CMakeLists.txt

file(GLOB SOURCE_FILES *.cpp)
file(GLOB HEADER_FILES *.h)

add_library(CoreTech_Common_Robot
	${SOURCE_FILES}
	${HEADER_FILES}
)
