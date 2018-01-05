/**
File: array2d_declarations.h
Author: Peter Barnum
Created: 2013

An Array is the basic large data structure for embedded work. It is designed for easy acceleration of algorithms on embedded hardware.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_ARRAY2D_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_ARRAY2D_DECLARATIONS_H_

//#include "coretech/common/robot/utilities_declarations.h"
#include "coretech/common/shared/types.h"
#include "coretech/common/robot/memory.h"
#include "coretech/common/robot/geometry_declarations.h"
#include "coretech/common/robot/utilities_c.h"
#include "coretech/common/robot/sequences_declarations.h"

//#if ANKICORETECH_EMBEDDED_USE_OPENCV
//namespace cv
//{
//  class Mat;
//  template<typename Type> class Mat_;
//}
//#endif

#include <opencv2/core.hpp>

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> class ArraySlice;
    template<typename Type> class ConstArraySlice;
    template<typename Type> class ConstArraySliceExpression;

    const s32 ARRAY_FILE_HEADER_LENGTH = 32;
    const s32 ARRAY_FILE_HEADER_VALID_LENGTH = 14; //< How many characters are not spaces
    const char ARRAY_FILE_HEADER[ARRAY_FILE_HEADER_LENGTH+1] = "\x89" "AnkiEArray1.2                  ";

    // #pragma mark --- Array Class Declaration ---

    template<typename Type> class Array
    {
    public:

      // The stride is the "numCols*sizeof(Type)" rounded up by 16, plus any boundary padding
      static s32 ComputeRequiredStride(const s32 numCols, const Flags::Buffer flags);

      // The minimum required memory is the size of a stride, times the number of rows
      static s32 ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const Flags::Buffer flags);

      // Initializes Array as invalid
      Array();

      // Constructor for a Array, pointing to user-allocated MemoryStack. This is the preferred
      // method for creating a new Array.
      //
      // Flags::Buffer.isFullyAllocated doesn't do anything
      Array(const s32 numRows, const s32 numCols, MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false,false));

      // Constructor for a Array, pointing to user-allocated data. This type of array is more
      // restrictive than most matrix libraries. For example, it may make it hard to convert from
      // OpenCV::Mat to Array, though the reverse is trivial.
      //
      // If following are true, then the contents of data will not be modified, and it will work as
      // a normal buffer without extra zeros as stride padding:
      // 1. (numCols*sizeof(Type)) % MEMORY_ALIGNMENT == 0
      // 2. reinterpret_cast<size_t>(data) % MEMORY_ALIGNMENT == 0
      // 3. numRows*numCols*sizeof(Type) <= dataLength
      //
      // If Flags::Buffer.isFullyAllocated == true, then the input data buffer's stride must be a
      // simple multiple
      Array(const s32 numRows, const s32 numCols, void * data, const s32 dataLength, const Flags::Buffer flags=Flags::Buffer(false,false,true));

      // Load an image from file. Requires OpenCV;
      static Array<Type> LoadImage(const char * filename, MemoryStack &memory);

      // Load or save an array saved as a debugStream.
      // compressionLevel can be from 0 (uncompressed) to 9 (most compressed). If OpenCV is not used, it must be zero.
      static Array<Type> LoadBinary(const char * filename, MemoryStack scratch, MemoryStack &memory);
      static Array<Type> LoadBinary(const char * filename, void * allocatedBuffer, const s32 allocatedBufferLength); //< allocatedBuffer must be allocated and freed manually
      Result SaveBinary(const char * filename, const s32 compressionLevel, MemoryStack scratch) const;

      // Pointer to the data, at a given (y,x) location
      //
      // NOTE:
      // Using this in a inner loop is very inefficient. Instead, declare a pointer outside the
      // inner loop, like: "Type * restrict pArray = Array.Pointer(5);", then index
      // pArray in the inner loop.
      inline const Type* Pointer(const s32 index0, const s32 index1) const;
      inline Type* Pointer(const s32 index0, const s32 index1);

      // Use this operator for normal C-style 2d matrix indexing. For example, "array[5][0] = 6;"
      // will set the element in the fifth row and first column to 6. This is the same as
      // "array.Pointer(5)[0] = 6;"
      //
      // NOTE:
      // Using this in a inner loop is very inefficient. Instead, declare a pointer outside the
      // inner loop, like: "Type * restrict pArray = Array[5];", then index
      // pArray in the inner loop.
      inline const Type * operator[](const s32 index0) const;
      inline Type * operator[](const s32 index0);

      // Pointer to the data, at a given (y,x) location
      //
      // NOTE:
      // The default order of coordinates for the Point() constructor is (x,y). So for example,
      // access Array[5][3] via Array.Pointer(Point<s16>(3,5))
      //
      // NOTE:
      // Using this in a inner loop is very inefficient. Instead, declare a pointer outside the
      // inner loop, like: "Type * restrict pArray = Array.Pointer(Point<s16>(5,0));", then index
      // pArray in the inner loop.
      inline const Type* Pointer(const Point<s16> &point) const;
      inline Type* Pointer(const Point<s16> &point);

      // Get the ith element, like Matlab's 1D indexing of a 2D array.
      // For example, the 5th element of Arrays of size (1,6) and (6,1) is the same;
      const Type& Element(const s32 elementIndex) const;
      Type& Element(const s32 elementIndex);

      // Return a slice accessor for this array, like the Matlab expression "array(1:5, 2:3:5)"
      //
      // NOTE:
      // If min or max is less than 0, it is equivalent to (end+value). For example, "Array(0,-1,3,5)"
      // is the same as "Array(0,arrayHeight-1,3,5)"
      ArraySlice<Type> operator() ();
      ArraySlice<Type> operator() (const LinearSequence<s32> &ySlice, const LinearSequence<s32> &xSlice);
      ArraySlice<Type> operator() (s32 minY, s32 maxY, s32 minX, s32 maxX);
      ArraySlice<Type> operator() (s32 minY, s32 incrementY, s32 maxY, s32 minX, s32 incrementX, s32 maxX);
      ConstArraySlice<Type> operator() () const;
      ConstArraySlice<Type> operator() (const LinearSequence<s32> &ySlice, const LinearSequence<s32> &xSlice) const;
      ConstArraySlice<Type> operator() (s32 minY, s32 maxY, s32 minX, s32 maxX) const;
      ConstArraySlice<Type> operator() (s32 minY, s32 incrementY, s32 maxY, s32 minX, s32 incrementX, s32 maxX) const;

      // ArraySlice Transpose doesn't modify the data, it just sets an "isTransposed" flag.
      // Anything that uses ArraySliceExpression respects this flag. This doesn't include things
      // like Matrix::Multiply(const Array<InType> &in1, const Array<InType> &in2, Array<OutType> &out) for example.
      ConstArraySliceExpression<Type> Transpose() const;

#if ANKICORETECH_EMBEDDED_USE_OPENCV
      // Copies the OpenCV Mat. If needed, it converts from color to grayscale by averaging the color channels.
      s32 Set(const cv::Mat_<Type> &in);
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV

      // Use the simple OpenCV gui to display this array as an image
      // Does nothing is OpenCV is not available
      void Show(const char * const windowName, const bool waitForKeypress, const bool scaleValues=false, const bool fitImageToWindow=false) const;

      // Print out the contents of this Array
      //
      // NOTE:
      // * If the min X or Y is less than zero, it will be treated as zero
      // * If the max X or Y is greater than the size of the array minus one, it will be treated as
      //   the size of the array minus one
      Result Print(const char * const variableName = "Array", const s32 minY = 0, const s32 maxY = 0x7FFFFFE, const s32 minX = 0, const s32 maxX = 0x7FFFFFE) const;
      Result PrintAlternate(const char * const variableName = "Array", const s32 version=2, const s32 minY = 0, const s32 maxY = 0x7FFFFFE, const s32 minX = 0, const s32 maxX = 0x7FFFFFE) const;

      // Checks if this array is equal to another array, up to some allowable
      // per-element varation, epsilon. If the arrays are not the same size,
      // false is returned.
      bool IsNearlyEqualTo(const Array<Type>& other, const Type epsilon) const;

      // Checks the basic parameters of this Array, and if it is allocated.
      bool IsValid() const;

      // Resize will use MemoryStack::Reallocate() to change the Array's size. It only works if this
      // Array was the last thing allocated. The reallocated memory will not be cleared
      //
      // WARNING:
      // This will not update any references to the memory, you must update all references manually.
      Result Resize(const s32 numRows, const s32 numCols, MemoryStack &memory);

      // Set every element in the Array to zero, including the stride padding.
      // Returns the number of bytes set to zero
      s32 SetZero();

      // Set every element in the Array to this value
      // Returns the number of values set
      s32 Set(const Type value);

      // Elementwise copies the input Array into this array. No memory is allocated.
      s32 Set(const Array<Type> &in);

      // Copy values to this Array.
      // If the input array does not contain enough elements, the remainder of this Array will be filled with zeros.
      // Returns the number of values set (not counting extra zeros)
      s32 Set(const Type * const values, const s32 numValues);

      // Read in the input, then cast it to this object's type
      //
      // WARNING:
      // This should be kept explicit, to prevent accidental casting between different datatypes.
      template<typename InType> s32 SetCast(const Array<InType> &in);
      template<typename InType> s32 SetCast(const InType * const values, const s32 numValues);

      // This is a shallow copy. There's no reference counting. Updating the data of one array will
      // update that of others (because they point to the same location in memory).
      // However, Resizing or other operations on one array won't update the others.
      Array& operator= (const Array & rightHandSide);

      // Similar to Matlabs size(matrix, dimension), and dimension is in {0,1}
      s32 get_size(s32 dimension) const;

      // Get the stride, which is the number of bytes between an element at (n,m) and an element at (n+1,m)
      s32 get_stride() const;

      // just size[0] * size[1]
      s32 get_numElements() const;

      // Return the flags that were used when this object was constructed.
      Flags::Buffer get_flags() const;

      // Equivalent to Pointer(0,0)
      //
      // These are for very low-level access to the buffers. Probably you want to be using one of
      // the Pointer() accessor methods instead of these.
      void* get_buffer();
      const void* get_buffer() const;

    protected:
      static const s32 HEADER_LENGTH = 8;
      static const s32 FOOTER_LENGTH = 8;

      s32 size[2];
      s32 stride;
      Flags::Buffer flags;

      Type * data;

      // Basic allocation method
      void* AllocateBufferFromMemoryStack(const s32 numRows, const s32 stride, MemoryStack &memory, s32 &numBytesAllocated, const Flags::Buffer flags, bool reAllocate);

      // Performs checks and sets appropriate parameters for this object
      Result InitializeBuffer(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const Flags::Buffer flags);

      // Set all the buffers and sizes to zero, to signal an invalid array
      void InvalidateArray();

      // If this object's Type is a basic type, this method prints out this object.
      Result PrintBasicType(const char * const variableName, const s32 version, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;

      // If this object's Type is a string, this method prints out this object.
      Result PrintString(const char * const variableName, const s32 version, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    }; // class Array

    // #pragma mark --- FixedPointArray Class Declaration ---

    template<typename Type> class FixedPointArray : public Array<Type>
    {
    public:
      FixedPointArray();

      // Same as Array() constructor
      // This is the preferred method for constructing an FixedPointArray
      FixedPointArray(const s32 numRows, const s32 numCols, const s32 numFractionalBits, MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false,false));

      // Same as Array() constructor
      // This is the advanced method for constructing an FixedPointArray
      FixedPointArray(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const s32 numFractionalBits, const Flags::Buffer flags=Flags::Buffer(true,false,false));

      s32 get_numFractionalBits() const;

    protected:
      s32 numFractionalBits;
    };

    // If you don't know the type of the Array you're loading, use this function directly, then cast it based on the read parameters
    Array<u8> LoadBinaryArray_UnknownType(
      const char * filename,
      MemoryStack *scratch,
      MemoryStack *memory,
      void * allocatedBuffer,
      const s32 allocatedBufferLength,
      u16  &basicType_sizeOfType,
      bool &basicType_isBasicType,
      bool &basicType_isInteger,
      bool &basicType_isSigned,
      bool &basicType_isFloat,
      bool &basicType_isString
      );

#if ANKICORETECH_EMBEDDED_USE_OPENCV
    // Returns a cv::Mat that mirrors the data in the input Array.
    // WARNING: If you copy the cv::Mat or assign it incorrectly, it will no longer mirror the input Array
    // WARNING: This const_casts the input array, so you can unsafely modify it via the output cv::Mat
    template<typename Type> Result ArrayToCvMat(const Array<Type> &in, cv::Mat *out);
#endif
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_ARRAY2D_DECLARATIONS_H_
