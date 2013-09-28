# coretech-vision/matlab/mex/CMakeLists.txt


set(MEX_LINK_LIBRARIES 
	CoreTech_Common CoreTech_Common_Embedded 
	CoreTech_Vision CoreTech_Vision_Embedded)
 
set(MEX_FILES
	mexBinomialFilter.cpp
	mexClosestIndex.cpp
	mexComputeCharacteristicScale.cpp
	mexDownsampleByFactor.cpp
	mexGaussianBlur.cpp
	mexHist.cpp
	mexRegionprops.cpp
	mexSimpleDetectorSteps123.cpp
	mexSimpleDetectorSteps1234.cpp
	mexTraceBoundary.cpp
	mex_cp2tform_projective.cpp
)

foreach(MEX_FILE ${MEX_FILES})
	build_mex(${MEX_FILE})	
endforeach()


