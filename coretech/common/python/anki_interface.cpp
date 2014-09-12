
#include <stdio.h>
#include <iostream>
#include <stdlib.h>

#include <Python.h>
#include <numpy/arrayobject.h>
#include <numpy/npy_common.h>
#include <numpy/ndarraytypes.h>

#include "anki/common/robot/array2d.h"

#include "anki.h"

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

PyMODINIT_FUNC initNumpy(void)
{
   import_array();
}

template<typename Type> PyObject* arrayToNumpyArray(const Array<Type> &in)
{
  const s32 inHeight = in.get_size(0);
  const s32 inWidth = in.get_size(1);
  
  npy_intp dims[2] = {inHeight, inWidth};
  const s32 numpyDataType = getNumpyDataType<Type>();
  
  PyObject * numpyArray = PyArray_SimpleNew(2, &dims[0], numpyDataType);
  
  u8 * restrict pPython_start = reinterpret_cast<u8*>( PyArray_DATA(numpyArray) );
  npy_intp *pythonStrides = PyArray_STRIDES(numpyArray);

  AnkiConditionalErrorAndReturnValue(sizeof(Type) == pythonStrides[1],
    NULL, "arrayToNumpyArray", "sizeof doesn't match stride");

  for(s32 y=0; y<inHeight; y++) {
    const Type * restrict pIn = in.Pointer(y,0);
    Type * restrict pPython = reinterpret_cast<Type*>( pPython_start + y * pythonStrides[0] );
    
    for(s32 x=0; x<inWidth; x++) {
      pPython[x] = pIn[x];
    }
  }
  
  return numpyArray;
}

PyObject* loadBinaryArray_toNumpy(const char * filename, const int maxBufferSize)
{
  initNumpy();
  
  AnkiConditionalErrorAndReturnValue(filename,
    NULL, "loadBinaryArray", "Invalid inputs");

  void * allocatedBuffer = malloc(maxBufferSize);

  AnkiConditionalErrorAndReturnValue(allocatedBuffer,
    NULL, "loadBinaryArray", "Could not allocate memory");

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
    AnkiError("loadBinaryArray", "Could not load array");
    
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
        toReturn = arrayToNumpyArray(array);
      } else if(basicType_sizeOfType == 8) {
        Array<f64> array = *reinterpret_cast<Array<f64>* >( &arrayTmp );
        toReturn = arrayToNumpyArray(array);
      } else {
        AnkiError("loadBinaryArray", "can only load a basic type");
      }
    } else { // if(basicType_isFloat)
      if(basicType_isSigned) {
        if(basicType_sizeOfType == 1) {
          Array<s8> array = *reinterpret_cast<Array<s8>* >( &arrayTmp );
          toReturn = arrayToNumpyArray(array);
        } else if(basicType_sizeOfType == 2) {
          Array<s16> array = *reinterpret_cast<Array<s16>* >( &arrayTmp );
          toReturn = arrayToNumpyArray(array);
        } else if(basicType_sizeOfType == 4) {
          Array<s32> array = *reinterpret_cast<Array<s32>* >( &arrayTmp );
          toReturn = arrayToNumpyArray(array);
        } else if(basicType_sizeOfType == 8) {
          Array<s64> array = *reinterpret_cast<Array<s64>* >( &arrayTmp );
          toReturn = arrayToNumpyArray(array);
        } else {
          AnkiError("loadBinaryArray", "can only load a basic type");
        }
      } else { // if(basicType_isSigned)
        if(basicType_sizeOfType == 1) {
          Array<u8> array = *reinterpret_cast<Array<u8>* >( &arrayTmp );
          toReturn = arrayToNumpyArray(array);
        } else if(basicType_sizeOfType == 2) {
          Array<u16> array = *reinterpret_cast<Array<u16>* >( &arrayTmp );
          toReturn = arrayToNumpyArray(array);
        } else if(basicType_sizeOfType == 4) {
          Array<u32> array = *reinterpret_cast<Array<u32>* >( &arrayTmp );
          toReturn = arrayToNumpyArray(array);
        } else if(basicType_sizeOfType == 8) {
          Array<u64> array = *reinterpret_cast<Array<u64>* >( &arrayTmp );
          toReturn = arrayToNumpyArray(array);
        } else {
          AnkiError("loadBinaryArray", "can only load a basic type");
        }
      } // if(basicType_isSigned) ... else
    } // if(basicType_isFloat) ... else
  } // if(basicType_isString) ... else

  if(allocatedBuffer)
    free(allocatedBuffer);

  return toReturn;

} // PyObject* LoadBinaryArray(const char * filename)

    
