# coretech/vision/robot/src/CMakeLists.txt

# Robot Vision Library

file(GLOB SOURCE_FILES "*.cpp")

file(GLOB HEADER_FILES "*.h")

add_library(CoreTech_Vision_Robot
	${SOURCE_FILES} 
	${HEADER_FILES}
)

