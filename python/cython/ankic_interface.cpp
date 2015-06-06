/**
File: anki_interface.cpp
Author: Peter Barnum
Created: 2014-09-12

Python utilities to load and save Embedded Arrays from a file. 
The three files ankic.pyx, ankic_interface.cpp, and ankic.h are compiled by setup.py,
to create a python-compatible shared library.

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <vector>

#include <Python.h>
#include <numpy/arrayobject.h>
#include <numpy/npy_common.h>
#include <numpy/ndarraytypes.h>

#include "anki/common/robot/array2d.h"
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/serialize.h"

#include "anki/vision/robot/lucasKanade.h"
#include "anki/vision/robot/fiducialMarkers.h"
#include "anki/vision/robot/fiducialDetection.h"

#include "ankic.h"

using namespace Anki;
using namespace Anki::Embedded;

template<typename Type> int getNumpyDataType() { return NPY_VOID; }

template<> int getNumpyDataType<u8>() { return NPY_UINT8; }
template<> int getNumpyDataType<s8>() { return NPY_INT8; }
template<> int getNumpyDataType<u16>() { return NPY_UINT16; }
template<> int getNumpyDataType<s16>() { return NPY_INT16; }
template<> int getNumpyDataType<u32>() { return NPY_UINT32; }
template<> int getNumpyDataType<s32>() { return NPY_INT32; }
template<> int getNumpyDataType<u64>() { return NPY_UINT64; }
template<> int getNumpyDataType<s64>() { return NPY_INT64; }
template<> int getNumpyDataType<f32>() { return NPY_FLOAT32; }
template<> int getNumpyDataType<f64>() { return NPY_FLOAT64; }

extern bool g_saveBinaryImage;
extern bool g_saveBinaryInitialized;
extern Anki::Embedded::Array<u8> g_binaryImage;

PyMODINIT_FUNC initNumpy(void)
{
   import_array();
}

template<typename Type> PyObject* ArrayToNumpyArray(const Array<Type> &in)
{
  const s32 inHeight = in.get_size(0);
  const s32 inWidth = in.get_size(1);

  npy_intp dims[2] = {inHeight, inWidth};
  const s32 numpyDataType = getNumpyDataType<Type>();

  PyObject * numpyArray = PyArray_SimpleNew(2, &dims[0], numpyDataType);

  AnkiConditionalErrorAndReturnValue(numpyArray,
    NULL, "ArrayToNumpyArray", "Could not allocate numpy array");

  u8 * restrict pNumpyArray_start = reinterpret_cast<u8*>( PyArray_DATA(numpyArray) );
  npy_intp *pythonStrides = PyArray_STRIDES(numpyArray);

  AnkiConditionalErrorAndReturnValue(sizeof(Type) == pythonStrides[1],
    NULL, "ArrayToNumpyArray", "sizeof doesn't match stride");

  for(s32 y=0; y<inHeight; y++) {
    const Type * restrict pIn = in.Pointer(y,0);
    Type * restrict pNumpyArray = reinterpret_cast<Type*>( pNumpyArray_start + y * pythonStrides[0] );

    for(s32 x=0; x<inWidth; x++) {
      pNumpyArray[x] = pIn[x];
    }
  }

  return numpyArray;
} // template<typename Type> PyObject* ArrayToNumpyArray(const Array<Type> &in)

PyObject* LoadEmbeddedArray_toNumpy(const char * filename)
{
  initNumpy();
  
  // TODO: vary based on the amount of memory needed
  const int maxBufferSize = 0x3fffffff;

  AnkiConditionalErrorAndReturnValue(filename,
    NULL, "LoadEmbeddedArray_toNumpy", "Invalid inputs");

  void * allocatedBuffer = malloc(maxBufferSize);

  AnkiConditionalErrorAndReturnValue(allocatedBuffer,
    NULL, "LoadEmbeddedArray_toNumpy", "Could not allocate memory");

  u16  basicType_sizeOfType = 0;
  bool basicType_isBasicType = false;
  bool basicType_isInteger = false;
  bool basicType_isSigned = false;
  bool basicType_isFloat = false;
  bool basicType_isString = false;

  Array<u8> arrayTmp = LoadBinaryArray_UnknownType(
    filename,
    NULL, NULL,
    allocatedBuffer, maxBufferSize,
    basicType_sizeOfType, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat, basicType_isString);

  if(!arrayTmp.IsValid()) {
    AnkiError("LoadEmbeddedArray_toNumpy", "Could not load array");

    if(allocatedBuffer)
      free(allocatedBuffer);

    return NULL;
  }

  PyObject * toReturn = NULL;
  if(basicType_isString) {
    // TODO:
  } else { // if(basicType_isString)
    if(basicType_isFloat) {
      if(basicType_sizeOfType == 4) {
        Array<f32> array = *reinterpret_cast<Array<f32>* >( &arrayTmp );
        toReturn = ArrayToNumpyArray(array);
      } else if(basicType_sizeOfType == 8) {
        Array<f64> array = *reinterpret_cast<Array<f64>* >( &arrayTmp );
        toReturn = ArrayToNumpyArray(array);
      } else {
        AnkiError("LoadEmbeddedArray_toNumpy", "can only load a basic type");
      }
    } else { // if(basicType_isFloat)
      if(basicType_isSigned) {
        if(basicType_sizeOfType == 1) {
          Array<s8> array = *reinterpret_cast<Array<s8>* >( &arrayTmp );
          toReturn = ArrayToNumpyArray(array);
        } else if(basicType_sizeOfType == 2) {
          Array<s16> array = *reinterpret_cast<Array<s16>* >( &arrayTmp );
          toReturn = ArrayToNumpyArray(array);
        } else if(basicType_sizeOfType == 4) {
          Array<s32> array = *reinterpret_cast<Array<s32>* >( &arrayTmp );
          toReturn = ArrayToNumpyArray(array);
        } else if(basicType_sizeOfType == 8) {
          Array<s64> array = *reinterpret_cast<Array<s64>* >( &arrayTmp );
          toReturn = ArrayToNumpyArray(array);
        } else {
          AnkiError("LoadEmbeddedArray_toNumpy", "can only load a basic type");
        }
      } else { // if(basicType_isSigned)
        if(basicType_sizeOfType == 1) {
          Array<u8> array = *reinterpret_cast<Array<u8>* >( &arrayTmp );
          toReturn = ArrayToNumpyArray(array);
        } else if(basicType_sizeOfType == 2) {
          Array<u16> array = *reinterpret_cast<Array<u16>* >( &arrayTmp );
          toReturn = ArrayToNumpyArray(array);
        } else if(basicType_sizeOfType == 4) {
          Array<u32> array = *reinterpret_cast<Array<u32>* >( &arrayTmp );
          toReturn = ArrayToNumpyArray(array);
        } else if(basicType_sizeOfType == 8) {
          Array<u64> array = *reinterpret_cast<Array<u64>* >( &arrayTmp );
          toReturn = ArrayToNumpyArray(array);
        } else {
          AnkiError("LoadEmbeddedArray_toNumpy", "can only load a basic type");
        }
      } // if(basicType_isSigned) ... else
    } // if(basicType_isFloat) ... else
  } // if(basicType_isString) ... else

  if(allocatedBuffer)
    free(allocatedBuffer);

  return toReturn;

} // PyObject* LoadEmbeddedArray_toNumpy(const char * filename)

template<typename Type> Array<Type> NumpyArrayToArray(const PyObject *numpyArray, MemoryStack &memory)
{
  const s32 arrayHeight = static_cast<s32>( PyArray_DIM(numpyArray, 0) );
  const s32 arrayWidth = static_cast<s32>( PyArray_DIM(numpyArray, 1) );

  Array<Type> array(arrayHeight, arrayWidth, memory);

  AnkiConditionalErrorAndReturnValue(array.IsValid(),
    Array<Type>(), "NumpyArrayToArray", "Could not allocate array");

  u8 * restrict pNumpyArray_start = reinterpret_cast<u8*>( PyArray_DATA(numpyArray) );
  npy_intp *pythonStrides = PyArray_STRIDES(numpyArray);

  AnkiConditionalErrorAndReturnValue(sizeof(Type) == pythonStrides[1],
    Array<Type>(), "NumpyArrayToArray", "sizeof doesn't match stride %d != %d", sizeof(Type), pythonStrides[1]);

  for(s32 y=0; y<arrayHeight; y++) {
    const Type * restrict pNumpyArray = reinterpret_cast<Type*>( pNumpyArray_start + y * pythonStrides[0] );
    Type * restrict pArray = array.Pointer(y,0);

    for(s32 x=0; x<arrayWidth; x++) {
      pArray[x] = pNumpyArray[x];
    }
  }

  return array;

} // template<typename Type> Array<Type> NumpyArrayToArray(const PyObject *numpyArray, MemoryStack &memory)

template<typename Type> s32 AllocateAndSave(const PyObject *numpyArray, const char *filename, const s32 compressionLevel)
{
  const s32 arrayHeight = static_cast<s32>( PyArray_DIM(numpyArray, 0) );
  const s32 arrayWidth = static_cast<s32>( PyArray_DIM(numpyArray, 1) );

  const s32 numArrayBytes = 8192 + arrayHeight * RoundUp<s32>(arrayWidth * sizeof(Type), MEMORY_ALIGNMENT);

  MemoryStack scratch1(malloc(numArrayBytes), numArrayBytes);
  MemoryStack scratch2(malloc(2*numArrayBytes), 2*numArrayBytes);

  if(!scratch1.IsValid() || !scratch2.IsValid()) {
    AnkiError("AllocateAndSave", "Out of memory");

    if(scratch1.get_buffer())
      free(scratch1.get_buffer());

    if(scratch2.get_buffer())
      free(scratch2.get_buffer());

      return -1;
  }

  Array<Type> ankiArray = NumpyArrayToArray<Type>(numpyArray, scratch1);

  if(!ankiArray.IsValid()) {
    free(scratch1.get_buffer());
    free(scratch2.get_buffer());

    return -1;
  }

  const Result result = ankiArray.SaveBinary(filename, compressionLevel, scratch2);

  free(scratch1.get_buffer());
  free(scratch2.get_buffer());

  if(result == RESULT_OK)
    return 0;
  else
    return -1;
} // template<typename Type> s32 AllocateAndSave(const PyObject *numpyArray, const char *filename, const s32 compressionLevel)

int SaveEmbeddedArray_fromNumpy(PyObject *numpyArray, const char *filename, const int compressionLevel)
{
  // TODO: support saving a lists of strings?

  initNumpy();

  if(!PyArray_Check(numpyArray)) {
    AnkiError("SaveEmbeddedArray_fromNumpy", "Input is not a Numpy array");
    return -1;
  }

  const s32 numpyDataType = PyArray_TYPE(numpyArray);

  if(numpyDataType == NPY_FLOAT64) {
    return AllocateAndSave<f64>(numpyArray, filename, compressionLevel);
  } else if(numpyDataType == NPY_FLOAT32) {
    return AllocateAndSave<f32>(numpyArray, filename, compressionLevel);
  } else if(numpyDataType == NPY_INT8) {
    return AllocateAndSave<s8>(numpyArray, filename, compressionLevel);
  } else if(numpyDataType == NPY_UINT8) {
    return AllocateAndSave<u8>(numpyArray, filename, compressionLevel);
  } else if(numpyDataType == NPY_INT16) {
    return AllocateAndSave<s16>(numpyArray, filename, compressionLevel);
  } else if(numpyDataType == NPY_UINT16) {
    return AllocateAndSave<u16>(numpyArray, filename, compressionLevel);
  } else if(numpyDataType == NPY_INT32) {
    return AllocateAndSave<s32>(numpyArray, filename, compressionLevel);
  } else if(numpyDataType == NPY_UINT32) {
    return AllocateAndSave<u32>(numpyArray, filename, compressionLevel);
  } else if(numpyDataType == NPY_INT64) {
    return AllocateAndSave<s64>(numpyArray, filename, compressionLevel);
  } else if(numpyDataType == NPY_UINT64) {
    return AllocateAndSave<u64>(numpyArray, filename, compressionLevel);
  } else {
    AnkiAssert(false);
  }

  return -1;
} // int SaveEmbeddedArray_fromNumpy(PyObject *numpyArray, const char *filename, const int compressionLevel)

PyObject* DetectFiducialMarkers_numpy(
  PyObject *numpyImage,
  const int useIntegralImageFiltering,
  const s32 scaleImage_numPyramidLevels,
  const s32 scaleImage_thresholdMultiplier,
  const s16 component1d_minComponentWidth,
  const s16 component1d_maxSkipDistance,
  const s32 component_minimumNumPixels,
  const s32 component_maximumNumPixels,
  const s32 component_sparseMultiplyThreshold,
  const s32 component_solidMultiplyThreshold,
  const f32 component_minHollowRatio,
  const s32 minLaplacianPeakRatio,
  const s32 quads_minQuadArea,
  const s32 quads_quadSymmetryThreshold,
  const s32 quads_minDistanceFromImageEdge,
  const f32 decode_minContrastRatio,
  const s32 quadRefinementIterations,
  const s32 numRefinementSamples,
  const f32 quadRefinementMaxCornerChange,
  const f32 quadRefinementMinCornerChange,
  const int returnInvalidMarkers)
{
  const s32 maxMarkers = 500;
  const s32 maxExtractedQuads = 5000;
  const s32 maxConnectedComponentSegments = 0xFFFFF; // 0xFFFFF is a little over one million
  const s32 bufferSize = 100000000;
  
  initNumpy();

  if(!PyArray_Check(numpyImage)) {
    AnkiError("DetectFiducialMarkers_numpy", "Input is not a Numpy array");
    return NULL;
  }

  const s32 numpyDataType = PyArray_TYPE(numpyImage);
  
  if(numpyDataType != NPY_UINT8) {
    AnkiError("DetectFiducialMarkers_numpy", "Input is not a uint8 Numpy array");
    return NULL;
  }
  
  MemoryStack scratch0(malloc(bufferSize), bufferSize);
  MemoryStack scratch1(malloc(bufferSize), bufferSize);
  MemoryStack scratch2(malloc(bufferSize), bufferSize);
  MemoryStack scratch3(malloc(bufferSize), bufferSize);

  if(!scratch0.IsValid() || !scratch1.IsValid() || !scratch2.IsValid() || !scratch3.IsValid()) {
    AnkiError("DetectFiducialMarkers_numpy", "Out of memory");

    if(scratch0.get_buffer())
      free(scratch0.get_buffer());

    if(scratch1.get_buffer())
      free(scratch1.get_buffer());
      
    if(scratch2.get_buffer())
      free(scratch2.get_buffer());
      
    if(scratch3.get_buffer())
      free(scratch3.get_buffer());            

    return NULL;
  }

  Array<u8> ankiImage = NumpyArrayToArray<u8>(numpyImage, scratch0);

  //printf("%d %d %d %d\n", *ankiImage.Pointer(10,10), *ankiImage.Pointer(20,10), *ankiImage.Pointer(30,10), *ankiImage.Pointer(10,30));

  if(!ankiImage.IsValid()) {
    free(scratch0.get_buffer());
    free(scratch1.get_buffer());
    free(scratch2.get_buffer());
    free(scratch3.get_buffer());

    return NULL;
  }
  
  FixedLengthList<VisionMarker> markers(maxMarkers, scratch0);
  FixedLengthList<Array<f32> > homographies(maxMarkers, scratch0);

  markers.set_size(maxMarkers);
  homographies.set_size(maxMarkers);

  for(s32 i=0; i<maxMarkers; i++) {
    Array<f32> newArray(3, 3, scratch0);
    homographies[i] = newArray;
  }
  
  {
    const Anki::Result result = DetectFiducialMarkers(
      ankiImage,
      markers,
      homographies,
      useIntegralImageFiltering,
      scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier,
      component1d_minComponentWidth, component1d_maxSkipDistance,
      component_minimumNumPixels, component_maximumNumPixels,
      component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
      component_minHollowRatio,
      minLaplacianPeakRatio,
      quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge,
      decode_minContrastRatio,
      maxConnectedComponentSegments,
      maxExtractedQuads,
      quadRefinementIterations,
      numRefinementSamples,
      quadRefinementMaxCornerChange,
      quadRefinementMinCornerChange,
      returnInvalidMarkers,
      scratch1,
      scratch2,
      scratch3);

    if(result != Anki::RESULT_OK) {
      AnkiError("DetectFiducialMarkers_numpy", "DetectFiducialMarkers failed 0x%x", result);
      
      free(scratch0.get_buffer());
      free(scratch1.get_buffer());
      free(scratch2.get_buffer());
      free(scratch3.get_buffer());

      return NULL;
    }
  }
  
  const s32 numMarkers = markers.get_size();

  PyObject* outputList = PyList_New(5);

  //printf("Outputting %d markers\n", numMarkers);

  if(numMarkers != 0) {
    // Output quads
    {
      PyObject* quads = PyList_New(numMarkers);
    
      for(s32 i=0; i<numMarkers; i++) {
        PyObject* curQuad = PyList_New(4);
      
        for(s32 j=0; j<4; j++) {
          PyObject* curPoint = PyList_New(2);
        
          PyList_SetItem(curPoint, 0, PyFloat_FromDouble(markers[i].corners[j].x));
          PyList_SetItem(curPoint, 1, PyFloat_FromDouble(markers[i].corners[j].y));
        
          PyList_SetItem(curQuad, j, curPoint);
        }
      
        PyList_SetItem(quads, i, curQuad);
      } // for(s32 i=0; i<numMarkers; i++)
  
      PyList_SetItem(outputList, 0, quads);
    } // Output quads
    
    // Marker types
    {
      PyObject* markerTypes = PyList_New(numMarkers);
      
      for(s32 i=0; i<numMarkers; i++) {
        PyList_SetItem(markerTypes, i, PyInt_FromLong(markers[i].markerType));
      } // for(s32 i=0; i<numMarkers; i++)
  
      PyList_SetItem(outputList, 1, markerTypes);
    } // Marker types
    
    // Marker type names
    {
      PyObject* markerNames = PyList_New(numMarkers);
      
      for(s32 i=0; i<numMarkers; i++) {
        AnkiAssert(markers[i].markerType >= 0 && markers[i].markerType <= Anki::Vision::NUM_MARKER_TYPES);
        
        PyList_SetItem(
          markerNames, 
          i, 
          PyString_FromStringAndSize(
            Anki::Vision::MarkerTypeStrings[markers[i].markerType], 
            strlen(Anki::Vision::MarkerTypeStrings[markers[i].markerType])));      
      } // for(s32 i=0; i<numMarkers; i++)
  
      PyList_SetItem(outputList, 2, markerNames);      
    } // Marker type names

    // Marker validity
    {
      PyObject* markerValidity = PyList_New(numMarkers);
      
      for(s32 i=0; i<numMarkers; i++) {
        PyList_SetItem(markerValidity, i, PyInt_FromLong(markers[i].validity));
      } // for(s32 i=0; i<numMarkers; i++)
  
      PyList_SetItem(outputList, 3, markerValidity);
    } // Marker validity
  } else { // if(numMarkers != 0)
    PyList_SetItem(outputList, 0, PyList_New(0));
    PyList_SetItem(outputList, 1, PyList_New(0));
    PyList_SetItem(outputList, 2, PyList_New(0));
    PyList_SetItem(outputList, 3, PyList_New(0));
  } // if(numMarkers != 0) ... else
    
  // Binary characteristic scale image (from computeCharacteristicScale.cpp)
  if(g_saveBinaryImage) {
    PyList_SetItem(outputList, 4, ArrayToNumpyArray<u8>(g_binaryImage));
  } else {
    PyList_SetItem(outputList, 4, PyList_New(0));
  }

  free(scratch0.get_buffer());
  free(scratch1.get_buffer());
  free(scratch2.get_buffer());
  free(scratch3.get_buffer());
  
  //printf("Reached the end\n");
  
  return outputList;
} // DetectFiducialMarkers_numpy()

