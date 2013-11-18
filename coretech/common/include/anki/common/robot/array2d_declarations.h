#ifndef _ANKICORETECHEMBEDDED_COMMON_ARRAY2D_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_ARRAY2D_DECLARATIONS_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities_declarations.h"
#include "anki/common/robot/memory.h"
#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/geometry_declarations.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/robot/cInterfaces_c.h"
#include "anki/common/robot/sequences_declarations.h"

#if ANKICORETECH_EMBEDDED_USE_OPENCV
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#endif

// #define ANKICORETECHEMBEDDED_ARRAY_STRING_INPUT

#define ANKI_ARRAY_USE_ARRAY_SET

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> class ArraySlice;
    template<typename Type> class ConstArraySlice;
    template<typename Type> class ConstArraySliceExpression;

    //template<typename Type1, typename Type2> class Find;

#pragma mark --- Array Class Declaration ---

    template<typename Type> class Array
    {
    public:
      static s32 ComputeRequiredStride(const s32 numCols, const Flags::Buffer flags);

      static s32 ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const Flags::Buffer flags);

      Array();

      // Constructor for a Array, pointing to user-allocated data. If the pointer to *data is not
      // aligned to MEMORY_ALIGNMENT, this Array will start at the next aligned location.
      // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
      // it may make it hard to convert from OpenCV to Array, though the reverse is trivial.
      // All memory in the array is zeroed out once it is allocated
      Array(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const Flags::Buffer flags=Flags::Buffer(true,false));

      // Constructor for a Array, pointing to user-allocated MemoryStack
      // All memory in the array is zeroed out once it is allocated
      Array(const s32 numRows, const s32 numCols, MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false));

      // Immediate evaluation of a LinearSequence, into this Array
      Array(const LinearSequence<Type> &sequence, MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false));

      // Pointer to the data, at a given (y,x) location
      //
      // NOTE:
      // Using this in a inner loop is very innefficient. Instead, declare a pointer outside the
      // inner loop, like: "Type * restrict pArray = Array.Pointer(5);", then index
      // pArray in the inner loop.
      inline const Type* Pointer(const s32 index0, const s32 index1) const;
      inline Type* Pointer(const s32 index0, const s32 index1);

      // Use this operator for normal C-style 2d matrix indexing. For example, "array[5][0] = 6;"
      // will set the element in the fifth row and first column to 6. This is the same as
      // "array.Pointer(5)[0] = 6;"
      //
      // NOTE:
      // Using this in a inner loop is very innefficient. Instead, declare a pointer outside the
      // inner loop, like: "Type * restrict pArray = Array[5];", then index
      // pArray in the inner loop.
      inline const Type * operator[](const s32 index0) const;
      inline Type * operator[](const s32 index0);

      // Pointer to the data, at a given (y,x) location
      //
      // NOTE:
      // Using this in a inner loop is very innefficient. Instead, declare a pointer outside the
      // inner loop, like: "Type * restrict pArray = Array.Pointer(Point<s16>(5,0));",
      // then index pArray in the inner loop.
      inline const Type* Pointer(const Point<s16> &point) const;
      inline Type* Pointer(const Point<s16> &point);

      // Return a slice accessor for this array, like the Matlab expression "array(1:5, 2:3:5)"
      ArraySlice<Type> operator() ();
      ArraySlice<Type> operator() (const LinearSequence<s32> &ySlice, const LinearSequence<s32> &xSlice);
      ArraySlice<Type> operator() (s32 minY, s32 maxY, s32 minX, s32 maxX); // If min or max is less than 0, it is equivalent to (end+value)
      ArraySlice<Type> operator() (s32 minY, s32 incrementY, s32 maxY, s32 minX, s32 incrementX, s32 maxX); // If min or max is less than 0, it is equivalent to (end+value)
      //operator ArraySlice<Type>(); // Implicit conversion

      ConstArraySlice<Type> operator() () const;
      ConstArraySlice<Type> operator() (const LinearSequence<s32> &ySlice, const LinearSequence<s32> &xSlice) const;
      ConstArraySlice<Type> operator() (s32 minY, s32 maxY, s32 minX, s32 maxX) const; // If min or max is less than 0, it is equivalent to (end+value)
      ConstArraySlice<Type> operator() (s32 minY, s32 incrementY, s32 maxY, s32 minX, s32 incrementX, s32 maxX) const; // If min or max is less than 0, it is equivalent to (end+value)
      //operator ConstArraySlice<Type>() const; // Implicit conversion

      // ArraySlice Transpose doesn't modify the data, it just sets a flag
      ConstArraySliceExpression<Type> Transpose() const;

#if ANKICORETECH_EMBEDDED_USE_OPENCV
      // Returns a templated cv::Mat_ that shares the same buffer with this Array. No data is copied.
      cv::Mat_<Type>& get_CvMat_();
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV

      void Show(const char * const windowName, const bool waitForKeypress, const bool scaleValues=false) const;

      // Print out the contents of this Array
      Result Print(const char * const variableName = "Array", const s32 minY = 0, const s32 maxY = 0x7FFFFFE, const s32 minX = 0, const s32 maxX = 0x7FFFFFE) const;

      // Checks the basic parameters of this Array.
      // If the Array was constructed with flags |= Flags::Buffer::USE_BOUNDARY_FILL_PATTERNS, then
      // return if any memory was written out of bounds (via fill patterns at the
      // beginning and end).
      bool IsValid() const;

      // Resize will use MemoryStack::Reallocate() to change the Array's size. It only works if this
      // Array was the last thing allocated. The reallocated memory will not be cleared
      //
      // WARNING: This will not update any references to the memory, you must update all references
      //          manually.
      Result Resize(const s32 numRows, const s32 numCols, MemoryStack &memory);

      // Set every element in the Array to zero, including the stride padding, but not including the optional fill patterns (if they exist)
      // Returns the number of bytes set to zero
      s32 SetZero();

      // Set every element in the Array to this value
      // Returns the number of values set
      s32 Set(const Type value);

      // Copy values to this Array.
      // If the input array does not contain enough elements, the remainder of this Array will be filled with zeros.
      // Returns the number of values set (not counting extra zeros)
      // Note: The myriad has many issues with static initialization of arrays, so this should be used with caution
#ifdef ANKI_ARRAY_USE_ARRAY_SET
      s32 Set_unsafe(const Type * const values, const s32 numValues);
      s32 Set(const s32 * const values, const s32 numValues);
      s32 Set(const f64 * const values, const s32 numValues);
#endif

      // Parse a space-seperated string, and copy values to this Array.
      // If the string does not contain enough elements, the remainder of the Array will be filled with zeros.
      // Returns the number of values set (not counting extra zeros)
#ifdef ANKICORETECHEMBEDDED_ARRAY_STRING_INPUT
      s32 Set(const char * const values);
#endif

      // TODO: implement all these
      //template<typename FindType1, typename FindType2> s32 Set(const Find<FindType1, FindType2> &find, const Type value);
      //template<typename FindType1, typename FindType2> s32 Set(const Find<FindType1, FindType2> &find, const Array<Type> &in, bool useFindForInput=false);
      //template<typename FindType1, typename FindType2> s32 Set(const Find<FindType1, FindType2> &find, const ConstArraySlice<Type> &in);

      // This is a shallow copy. There's no reference counting. Updating the data of one array will
      // update that of others (because they point to the same location in memory). However,
      // Resizing or other operations on an array won't update the others.
      Array& operator= (const Array & rightHandSide);

      // Similar to Matlabs size(matrix, dimension), and dimension is in {0,1}
      s32 get_size(s32 dimension) const;

      // Get the stride, which is the number of bytes between an element at (n,m) and one at (n+1,m)
      s32 get_stride() const;

      // Get the stride, without the optional fill pattern. This is the number of bytes on a
      // horizontal line that can safely be read and written to. If this array was created without
      // fill patterns, it returns the same value as get_stride().
      s32 get_strideWithoutFillPatterns() const;

      void* get_rawDataPointer();

      const void* get_rawDataPointer() const;

      Flags::Buffer get_flags() const;

    protected:
      // Bit-inverse of MemoryStack patterns. The pattern will be put twice at
      // the beginning and end of each line.
      static const u32 FILL_PATTERN_START = 0X5432EF76;
      static const u32 FILL_PATTERN_END = 0X7610FE76;

      static const s32 HEADER_LENGTH = 8;
      static const s32 FOOTER_LENGTH = 8;

      s32 size[2];
      s32 stride;
      Flags::Buffer flags;

      Type * data;

      // To enforce alignment, rawDataPointer may be slightly before Type * data.
      // If the inputted data buffer was from malloc, this is the pointer that
      // should be used to free.
      void * rawDataPointer;

#if ANKICORETECH_EMBEDDED_USE_OPENCV
      cv::Mat_<Type> cvMatMirror;
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV

      void* AllocateBufferFromMemoryStack(const s32 numRows, const s32 stride, MemoryStack &memory, s32 &numBytesAllocated, const Flags::Buffer flags, bool reAllocate);

      Result InitializeBuffer(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const Flags::Buffer flags);

      void InvalidateArray(); // Set all the buffers and sizes to zero, to signal an invalid array

      Result PrintBasicType(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    }; // class Array

#pragma mark --- FixedPointArray Class Declaration ---

    template<typename Type> class FixedPointArray : public Array<Type>
    {
    public:
      FixedPointArray();

      // Constructor for a Array, pointing to user-allocated data. If the pointer to *data is not
      // aligned to MEMORY_ALIGNMENT, this Array will start at the next aligned location.
      // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
      // it may make it hard to convert from OpenCV to Array, though the reverse is trivial.
      // All memory in the array is zeroed out once it is allocated
      FixedPointArray(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const s32 numFractionalBits, const Flags::Buffer flags=Flags::Buffer(true,false));

      // Constructor for a Array, pointing to user-allocated MemoryStack
      // All memory in the array is zeroed out once it is allocated
      FixedPointArray(const s32 numRows, const s32 numCols, const s32 numFractionalBits, MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false));

      s32 get_numFractionalBits() const;

    protected:
      s32 numFractionalBits;
    };
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_ARRAY2D_DECLARATIONS_H_
