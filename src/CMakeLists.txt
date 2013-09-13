# coretech-vision/src/CMakeLists.txt

# Vision Library
#message("CORETECH_VISION_LIBNAME has been set to ${CORETECH_VISION_LIBNAME}")

file(GLOB CORETECH_VISION_FILES "*.cpp")

add_library(CoreTech_Vision ${CORETECH_VISION_FILES})
target_link_libraries(CoreTech_Vision)


# Vision Embedded Library
#message("CORETECH_VISION_LIBNAME_EMBEDDED has been set to ${CORETECH_VISION_LIBNAME_EMBEDDED}")

file(GLOB CORETECH_VISION_EMBEDDED_FILES "embedded/*.cpp")

add_library(CoreTech_Vision_Embedded
	${CORETECH_VISION_FILES}
	${CORETECH_VISION_EMBEDDED_FILES})

