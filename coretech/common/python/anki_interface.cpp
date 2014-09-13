
#include <stdio.h>
#include <iostream>
#include <stdlib.h>

#include <Python.h>
#include <numpy/arrayobject.h>
#include <numpy/npy_common.h>
#include <numpy/ndarraytypes.h>

#include "anki/common/robot/array2d.h"
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/serialize.h"

#include "anki.h"

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
  
  u8 * restrict pNumpyArray_start = reinterpret_cast<u8*>( PyArray_DATA(numpyArray) );
  npy_intp *pythonStrides = PyArray_STRIDES(numpyArray);

  AnkiConditionalErrorAndReturnValue(sizeof(Type) == pythonStrides[1],
    NULL, "arrayToNumpyArray", "sizeof doesn't match stride");

  for(s32 y=0; y<inHeight; y++) {
    const Type * restrict pIn = in.Pointer(y,0);
    Type * restrict pNumpyArray = reinterpret_cast<Type*>( pNumpyArray_start + y * pythonStrides[0] );
    
    for(s32 x=0; x<inWidth; x++) {
      pNumpyArray[x] = pIn[x];
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
    Array<Type>(), "arrayToNumpyArray", "sizeof doesn't match stride");

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
} // template<typename Type> Result AllocateAndSave(const PyObject *numpyArray, const char *filename, const s32 compressionLevel)

int saveBinaryArray_fromNumpy(PyObject *numpyArray, const char *filename, const int compressionLevel)
{
  // TODO: check if it is actually a numpy array

  const s32 numpyDataType = PyArray_TYPE(numpyArray);

  // TODO: save lists of strings?
  
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
}
    
    
