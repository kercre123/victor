# coretech-common/src/CMakeLists.txt


# Common Library
file(GLOB CORETECH_COMMON_FILES "*.cpp")

add_library(CoreTech_Common ${CORETECH_COMMON_FILES})


# Common Embedded Library
file(GLOB CORETECH_COMMON_EMBEDDED_FILES "embedded/*.cpp")

add_library(CoreTech_Common_Embedded 
	${CORETECH_COMMON_FILES}
	${CORETECH_COMMON_EMBEDDED_FILES})


