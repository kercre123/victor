# coretech/CMakeLists.txt

cmake_minimum_required(VERSION 2.8)

include(anki.cmake)

include_directories(
	common/include
	vision/include
	external/include
)
	
ankiProject(CoreTech)

add_subdirectory(common)
add_subdirectory(vision)
#add_subdirectory(external)