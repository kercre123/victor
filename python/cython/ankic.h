/**
File: anki.h
Author: Peter Barnum
Created: 2014-09-12

Python utilities to load and save Embedded Arrays from a file. 
The three files anki.pyx, anki_interface.cpp, and anki.h are compiled by setup.py,
to create a python-compatible shared library.

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

PyObject* LoadEmbeddedArray_toNumpy(
  const char * filename);

int SaveEmbeddedArray_fromNumpy(
  PyObject *numpyArray, 
  const char *filename, 
  const int compressionLevel);

PyObject* DetectFiducialMarkers_numpy(
  PyObject *numpyImage,
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
  const int returnInvalidMarkers);
