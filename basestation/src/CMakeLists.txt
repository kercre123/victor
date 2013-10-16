# products-cozmo/basestation/src/CMakeLists.txt

file(GLOB SOURCE_FILES "*.cpp")
file(GLOB PRIVATE_HEADER_FILES "*.h")
file(GLOB PUBLIC_HEADER_FILES "${COZMO_INCLUDE_DIR}/anki/cozmo/basestation/*.h")

add_library(CozmoBasestation 
	${SOURCE_FILES} 
	${PRIVATE_HEADER_FILES}
	${PUBLIC_HEADER_FILES}
	${COZMO_HEADER_FILES}
)

target_link_libraries(CozmoBasestation 
	CoreTech_Common_Basestation 
	CoreTech_Vision_Basestation 
	${ZLIB_LIBRARY}
)
