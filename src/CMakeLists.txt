# coretech-common/src/CMakeLists.txt


# Common Library
#message("CORETECH_COMMON_LIBNAME has been set to ${CORETECH_COMMON_LIBNAME}")

file(GLOB CORETECH_COMMON_FILES "*.cpp")

add_library(CoreTech_Common ${CORETECH_COMMON_FILES})


# Common Embedded Library

#message("CORETECH_COMMON_LIBNAME_EMBEDDED has been set to ${CORETECH_COMMON_LIBNAME_EMBEDDED}")

file(GLOB CORETECH_COMMON_EMBEDDED_FILES "embedded/*.cpp")

add_library(CoreTech_Common_Embedded 
	${CORETECH_COMMON_FILES}
	${CORETECH_COMMON_EMBEDDED_FILES})


