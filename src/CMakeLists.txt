# coretech-common/src/CMakeLists.txt


# Common Library
#file(GLOB CORETECH_COMMON_FILES "*.cpp")

set(CORETECH_COMMON_SOURCE_FILES
	array2d.cpp
	general.cpp
	matrix.cpp
	memory.cpp
	point.cpp
	pose.cpp
	quad.cpp
	radians.cpp
	rotation.cpp
)

set(CORETECH_COMMON_HEADER_FILES
	../include/anki/common.h
	../include/anki/common/array2d.h
	../include/anki/common/exceptions.h
	../include/anki/common/general.h
	../include/anki/math/matrix.h
	../include/anki/math/point.h
	../include/anki/math/pose.h
	../include/anki/math/quad.h
	../include/anki/math/radians.h
	../include/anki/math/rotation.h
)
add_library(CoreTech_Common 

	${CORETECH_COMMON_SOURCE_FILES}
	${CORETECH_COMMON_HEADER_FILES}
)


# Common Embedded Library
#file(GLOB CORETECH_COMMON_EMBEDDED_FILES "embedded/*.cpp")
set(CORETECH_COMMON_EMBEDDED_SOURCE_FILES
	embedded/array2d.cpp
	embedded/benchmarking_c.c
	embedded/utilities_c.c
	embedded/draw.cpp
	embedded/errorHandling.cpp
	embedded/fixedLengthList.cpp
	embedded/geometry.cpp
	embedded/matlabInterface.cpp
	embedded/memory.cpp
	embedded/opencvLight.cpp
	embedded/utilities.cpp
)

set(CORETECH_COMMON_EMBEDDED_HEADER_FILES
	../include/anki/embeddedCommon.h
	../include/anki/embeddedCommon/array2d.h
	../include/anki/embeddedCommon/benchmarking_c.h
	../include/anki/embeddedCommon/config.h
	../include/anki/embeddedCommon/dataStructures.h
	../include/anki/embeddedCommon/draw.h
	../include/anki/embeddedCommon/errorHandling.h
	../include/anki/embeddedCommon/fixedLengthList.h
	../include/anki/embeddedCommon/geometry.h
	../include/anki/embeddedCommon/gtestLight.h
	../include/anki/embeddedCommon/matlabInterface.h
	../include/anki/embeddedCommon/memory.h
	../include/anki/embeddedCommon/opencvLight.h
	../include/anki/embeddedCommon/utilities.h
	../include/anki/embeddedCommon/utilities_c.h
)

add_library(CoreTech_Common_Embedded 
	${CORETECH_COMMON_EMBEDDED_SOURCE_FILES}
	${CORETECH_COMMON_EMBEDDED_HEADER_FILES}
)


