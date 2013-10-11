# coretech-vision/src/CMakeLists.txt

# Vision Library
#message("CORETECH_VISION_LIBNAME has been set to ${CORETECH_VISION_LIBNAME}")

#file(GLOB CORETECH_VISION_FILES "*.cpp")
set(CORETECH_VISION_SOURCE_FILES
  camera.cpp
  image.cpp
  marker2d.cpp
)

set(CORETECH_VISION_HEADER_FILES
  ../include/anki/vision.h
  ../include/anki/vision/camera.h
  ../include/anki/vision/config.h
  ../include/anki/vision/image.h
  ../include/anki/vision/marker2d.h
)

add_library(CoreTech_Vision 
  ${CORETECH_VISION_SOURCE_FILES} 
  ${CORETECH_VISION_HEADER_FILES})

target_link_libraries(CoreTech_Vision)


# Vision Embedded Library
#message("CORETECH_VISION_LIBNAME_EMBEDDED has been set to ${CORETECH_VISION_LIBNAME_EMBEDDED}")

# file(GLOB CORETECH_VISION_EMBEDDED_FILES "embedded/*.cpp")
set(CORETECH_VISION_EMBEDDED_FILES
	embedded/array2d_vision.cpp
	embedded/binomialFilter.cpp
	embedded/computeCharacteristicScale.cpp
	embedded/computeQuadrilaterals.cpp
	embedded/connectedComponents.cpp
	embedded/dataStructures_vision.cpp
	embedded/fiducialMarkers.cpp
	embedded/downsampleByFactor.cpp
	embedded/draw_vision.cpp
	embedded/filtering.cpp
	embedded/fixedLengthList_vision.cpp
	embedded/matlabPrototyping.cpp
	embedded/thresholdScaleImage.cpp
	embedded/traceBoundary.cpp
	embedded/transformations.cpp
)

set(CORETECH_VISION_EMBEDDED_HEADER_FILES
	../include/anki/embeddedVision.h
	../include/anki/embeddedVision/array2d_vision.h
	../include/anki/embeddedVision/config.h
	../include/anki/embeddedVision/connectedComponents.h
	../include/anki/embeddedVision/dataStructures_vision.h
	../include/anki/embeddedVision/draw_vision.h
	embedded/fiducialMarkerDefinitionType0.h
	../include/anki/embeddedVision/fiducialMarkers.h
	../include/anki/embeddedVision/fixedLengthList_vision.h	
	../include/anki/embeddedVision/miscVisionKernels.h
)

add_library(CoreTech_Vision_Embedded
	${CORETECH_VISION_EMBEDDED_FILES} ${CORETECH_VISION_EMBEDDED_HEADER_FILES})

