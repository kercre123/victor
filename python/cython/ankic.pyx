# distutils: language = c++

cdef extern from "ankic.h":
  object LoadEmbeddedArray_toNumpy(
    const char * filename)
  
  int SaveEmbeddedArray_fromNumpy(
    object numpyArray, 
    const char *filename, 
    const int compressionLevel) except -1
  
  object DetectFiducialMarkers_numpy(
    object numpyImage,
    const int useIntegralImageFiltering,
    const int scaleImage_numPyramidLevels,
    const int scaleImage_thresholdMultiplier,
    const short component1d_minComponentWidth,
    const short component1d_maxSkipDistance,
    const int component_minimumNumPixels,
    const int component_maximumNumPixels,
    const int component_sparseMultiplyThreshold,
    const int component_solidMultiplyThreshold,
    const float component_minHollowRatio,
    const int minLaplacianPeakRatio,
    const int quads_minQuadArea,
    const int quads_quadSymmetryThreshold,
    const int quads_minDistanceFromImageEdge,
    const float decode_minContrastRatio,
    const int quadRefinementIterations,
    const int numRefinementSamples,
    const float quadRefinementMaxCornerChange,
    const float quadRefinementMinCornerChange,
    const int returnInvalidMarkers)

def loadEmbeddedArray(const char * filename):
  """
  Load the file "filename" into a numpy array. 
  """
  
  try:
    return LoadEmbeddedArray_toNumpy(filename)
  except:
    raise Exception("Could not load from " + filename)  
    
  return None

def saveEmbeddedArray(object numpyArray, const char * filename, const int compressionLevel):
  """
  Save the numpy array "numpyArray" to "filename"
  "compressionLevel" can be from 0 (no compression) to 9 (best compression). 6 is a good choice.
  """
  
  try:
    SaveEmbeddedArray_fromNumpy(numpyArray, filename, compressionLevel)
  except:
    raise Exception("Could not save to " + filename)
    
def detectFiducialMarkers(object numpyImage, const int useIntegralImageFiltering, const int scaleImage_numPyramidLevels, const float scaleImage_thresholdMultiplierF32, const short component1d_minComponentWidth, const short component1d_maxSkipDistance, const int component_minimumNumPixels, const int component_maximumNumPixels, const float component_sparseMultiplyThresholdF32, const float component_solidMultiplyThresholdF32, const float component_minHollowRatio, const int minLaplacianPeakRatio, const int quads_minQuadArea, const float quads_quadSymmetryThresholdF32, const int quads_minDistanceFromImageEdge, const float decode_minContrastRatio, const int quadRefinementIterations, const int numRefinementSamples, const float quadRefinementMaxCornerChange, const float quadRefinementMinCornerChange, const int returnInvalidMarkers):
  """
  Detect fiducial markers in image, and return the markers
  
  Example:
  import numpy as np
  import ankic
  import cv2
  image = cv2.imread('/Users/pbarnum/Documents/Anki/products-cozmo-large-files/systemTestsData/trackingImages/tracking_00000_cozmo1_img_544810.jpg')
  image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

  imageSize = image.shape
  useIntegralImageFiltering = True
  scaleImage_thresholdMultiplier = 1.0
  scaleImage_numPyramidLevels = 3
  component1d_minComponentWidth = 0
  component1d_maxSkipDistance = 0
  minSideLength = round(0.01*max(imageSize[0],imageSize[1]))
  maxSideLength = round(0.97*min(imageSize[0],imageSize[1]))
  component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength))
  component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength))
  component_sparseMultiplyThreshold = 1000.0
  component_solidMultiplyThreshold = 2.0
  component_minHollowRatio = 1.0
  minLaplacianPeakRatio = 5;
  quads_minQuadArea = 100 / 4
  quads_quadSymmetryThreshold = 2.0
  quads_minDistanceFromImageEdge = 2
  decode_minContrastRatio = 1.25
  quadRefinementIterations = 5
  numRefinementSamples = 100
  quadRefinementMaxCornerChange = 5.0
  quadRefinementMinCornerChange = 0.005
  returnInvalidMarkers = 0
  quads, markerTypes, markerNames, markerValidity, binaryImage = ankic.detectFiducialMarkers(image, useIntegralImageFiltering, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_minHollowRatio, minLaplacianPeakRatio, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio, quadRefinementIterations, numRefinementSamples, quadRefinementMaxCornerChange, quadRefinementMinCornerChange, returnInvalidMarkers)
  """

  scaleImage_thresholdMultiplierS32    = int(round(pow(2,16)*scaleImage_thresholdMultiplierF32)) 
  component_sparseMultiplyThresholdS32 = int(round(pow(2,5)*component_sparseMultiplyThresholdF32))
  component_solidMultiplyThresholdS32  = int(round(pow(2,5)*component_solidMultiplyThresholdF32))
  quads_quadSymmetryThresholdS32       = int(round(pow(2,8)*quads_quadSymmetryThresholdF32))

  try:
    return DetectFiducialMarkers_numpy(numpyImage, useIntegralImageFiltering, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplierS32, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThresholdS32, component_solidMultiplyThresholdS32, component_minHollowRatio, minLaplacianPeakRatio, quads_minQuadArea, quads_quadSymmetryThresholdS32, quads_minDistanceFromImageEdge, decode_minContrastRatio, quadRefinementIterations, numRefinementSamples, quadRefinementMaxCornerChange, quadRefinementMinCornerChange, returnInvalidMarkers)
  except:
    raise Exception("Could not detect fiducial markers")
