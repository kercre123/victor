#ifndef _ANKICORETECHEMBEDDED_COMMON_ARRAY2D_H_
#define _ANKICORETECHEMBEDDED_COMMON_ARRAY2D_H_

#include "anki/embeddedCommon/config.h"
#include "anki/embeddedCommon/utilities.h"
#include "anki/embeddedCommon/memory.h"
#include "anki/embeddedCommon/DASlight.h"
#include "anki/embeddedCommon/dataStructures.h"
#include "anki/embeddedCommon/point.h"

//#include <iostream>
#include <assert.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

namespace Anki
{
  namespace Embedded
  {
    class Array_u8
    {
    public:
      static s32 ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns);

      static s32 ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns);

      Array_u8();

      // Constructor for a Array_u8, pointing to user-allocated data. If the pointer to *data is not
      // aligned to MEMORY_ALIGNMENT, this Array_u8 will start at the next aligned location.
      // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
      // it may make it hard to convert from OpenCV to Array_u8, though the reverse is trivial.
      Array_u8(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const bool useBoundaryFillPatterns=false);

      // Constructor for a Array_u8, pointing to user-allocated MemoryStack
      Array_u8(const s32 numRows, const s32 numCols, MemoryStack &memory, const bool useBoundaryFillPatterns=false);

      // Pointer to the data, at a given (y,x) location
      inline const u8* Array_u8::Pointer(const s32 index0, const s32 index1) const
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<const u8*>( reinterpret_cast<const char*>(this->data) +
          index1*sizeof(u8) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline u8* Array_u8::Pointer(const s32 index0, const s32 index1)
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<u8*>( reinterpret_cast<char*>(this->data) +
          index1*sizeof(u8) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline const u8* Array_u8::Pointer(const Point_s16 &point) const
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

      // Pointer to the data, at a given (y,x) location
      inline u8* Array_u8::Pointer(const Point_s16 &point)
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      void Show(const char * const windowName, const bool waitForKeypress) const;

      // Returns a templated cv::Mat_ that shares the same buffer with this Array_u8. No data is copied.
      cv::Mat_<u8>& get_CvMat_();
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

      // Print out the contents of this Array_u8
      void Print() const;

      // If the Array_u8 was constructed with the useBoundaryFillPatterns=true, then
      // return if any memory was written out of bounds (via fill patterns at the
      // beginning and end).  If the Array_u8 wasn't constructed with the
      // useBoundaryFillPatterns=true, this method always returns true
      bool IsValid() const;

      // Set every element in the Array_u8 to this value
      // Returns the number of values set
      s32 Set(const u8 value);

      // Parse a space-seperated string, and copy values to this Array_u8.
      // If the string doesn't contain enough elements, the remainder of the Array_u8 will be filled with zeros.
      // Returns the number of values set (not counting extra zeros)
      s32 Set(const char * const values);

      // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
      s32 get_size(s32 dimension) const;

      s32 get_stride() const;

      void* get_rawDataPointer();

      const void* get_rawDataPointer() const;

    protected:
      // Bit-inverse of MemoryStack patterns. The pattern will be put twice at
      // the beginning and end of each line.
      static const u32 FILL_PATTERN_START = 0X5432EF76;
      static const u32 FILL_PATTERN_END = 0X7610FE76;

      static const s32 HEADER_LENGTH = 8;
      static const s32 FOOTER_LENGTH = 8;

      s32 size[2];
      s32 stride;
      bool useBoundaryFillPatterns;

      u8 * data;

      // To enforce alignment, rawDataPointer may be slightly before u8 * data.
      // If the inputted data buffer was from malloc, this is the pointer that
      // should be used to free.
      void * rawDataPointer;

      void initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns);

      void invalidateArray(); // Set all the buffers and sizes to zero, to signal an invalid array

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cv::Mat_<u8> cvMatMirror;
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    private:
      //Array_u8 & operator= (const Array_u8 & rightHandSide); // Not allowed
    }; // class Array_u8

    class Array_s8
    {
    public:
      static s32 ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns);

      static s32 ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns);

      Array_s8();

      // Constructor for a Array_s8, pointing to user-allocated data. If the pointer to *data is not
      // aligned to MEMORY_ALIGNMENT, this Array_s8 will start at the next aligned location.
      // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
      // it may make it hard to convert from OpenCV to Array_s8, though the reverse is trivial.
      Array_s8(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const bool useBoundaryFillPatterns=false);

      // Constructor for a Array_s8, pointing to user-allocated MemoryStack
      Array_s8(const s32 numRows, const s32 numCols, MemoryStack &memory, const bool useBoundaryFillPatterns=false);

      // Pointer to the data, at a given (y,x) location
      inline const s8* Array_s8::Pointer(const s32 index0, const s32 index1) const
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<const s8*>( reinterpret_cast<const char*>(this->data) +
          index1*sizeof(s8) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline s8* Array_s8::Pointer(const s32 index0, const s32 index1)
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<s8*>( reinterpret_cast<char*>(this->data) +
          index1*sizeof(s8) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline const s8* Array_s8::Pointer(const Point_s16 &point) const
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

      // Pointer to the data, at a given (y,x) location
      inline s8* Array_s8::Pointer(const Point_s16 &point)
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      void Show(const char * const windowName, const bool waitForKeypress) const;

      // Returns a templated cv::Mat_ that shares the same buffer with this Array_s8. No data is copied.
      cv::Mat_<s8>& get_CvMat_();
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

      // Print out the contents of this Array_s8
      void Print() const;

      // If the Array_s8 was constructed with the useBoundaryFillPatterns=true, then
      // return if any memory was written out of bounds (via fill patterns at the
      // beginning and end).  If the Array_s8 wasn't constructed with the
      // useBoundaryFillPatterns=true, this method always returns true
      bool IsValid() const;

      // Set every element in the Array_s8 to this value
      // Returns the number of values set
      s32 Set(const s8 value);

      // Parse a space-seperated string, and copy values to this Array_s8.
      // If the string doesn't contain enough elements, the remainder of the Array_s8 will be filled with zeros.
      // Returns the number of values set (not counting extra zeros)
      s32 Set(const char * const values);

      // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
      s32 get_size(s32 dimension) const;

      s32 get_stride() const;

      void* get_rawDataPointer();

      const void* get_rawDataPointer() const;

    protected:
      // Bit-inverse of MemoryStack patterns. The pattern will be put twice at
      // the beginning and end of each line.
      static const u32 FILL_PATTERN_START = 0X5432EF76;
      static const u32 FILL_PATTERN_END = 0X7610FE76;

      static const s32 HEADER_LENGTH = 8;
      static const s32 FOOTER_LENGTH = 8;

      s32 size[2];
      s32 stride;
      bool useBoundaryFillPatterns;

      s8 * data;

      // To enforce alignment, rawDataPointer may be slightly before s8 * data.
      // If the inputted data buffer was from malloc, this is the pointer that
      // should be used to free.
      void * rawDataPointer;

      void initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns);

      void invalidateArray(); // Set all the buffers and sizes to zero, to signal an invalid array

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cv::Mat_<s8> cvMatMirror;
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    private:
      //Array_s8 & operator= (const Array_s8 & rightHandSide); // Not allowed
    }; // class Array_s8

    class Array_u16
    {
    public:
      static s32 ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns);

      static s32 ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns);

      Array_u16();

      // Constructor for a Array_u16, pointing to user-allocated data. If the pointer to *data is not
      // aligned to MEMORY_ALIGNMENT, this Array_u16 will start at the next aligned location.
      // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
      // it may make it hard to convert from OpenCV to Array_u16, though the reverse is trivial.
      Array_u16(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const bool useBoundaryFillPatterns=false);

      // Constructor for a Array_u16, pointing to user-allocated MemoryStack
      Array_u16(const s32 numRows, const s32 numCols, MemoryStack &memory, const bool useBoundaryFillPatterns=false);

      // Pointer to the data, at a given (y,x) location
      inline const u16* Array_u16::Pointer(const s32 index0, const s32 index1) const
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<const u16*>( reinterpret_cast<const char*>(this->data) +
          index1*sizeof(u16) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline u16* Array_u16::Pointer(const s32 index0, const s32 index1)
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<u16*>( reinterpret_cast<char*>(this->data) +
          index1*sizeof(u16) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline const u16* Array_u16::Pointer(const Point_s16 &point) const
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

      // Pointer to the data, at a given (y,x) location
      inline u16* Array_u16::Pointer(const Point_s16 &point)
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      void Show(const char * const windowName, const bool waitForKeypress) const;

      // Returns a templated cv::Mat_ that shares the same buffer with this Array_u16. No data is copied.
      cv::Mat_<u16>& get_CvMat_();
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

      // Print out the contents of this Array_u16
      void Print() const;

      // If the Array_u16 was constructed with the useBoundaryFillPatterns=true, then
      // return if any memory was written out of bounds (via fill patterns at the
      // beginning and end).  If the Array_u16 wasn't constructed with the
      // useBoundaryFillPatterns=true, this method always returns true
      bool IsValid() const;

      // Set every element in the Array_u16 to this value
      // Returns the number of values set
      s32 Set(const u16 value);

      // Parse a space-seperated string, and copy values to this Array_u16.
      // If the string doesn't contain enough elements, the remainder of the Array_u16 will be filled with zeros.
      // Returns the number of values set (not counting extra zeros)
      s32 Set(const char * const values);

      // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
      s32 get_size(s32 dimension) const;

      s32 get_stride() const;

      void* get_rawDataPointer();

      const void* get_rawDataPointer() const;

    protected:
      // Bit-inverse of MemoryStack patterns. The pattern will be put twice at
      // the beginning and end of each line.
      static const u32 FILL_PATTERN_START = 0X5432EF76;
      static const u32 FILL_PATTERN_END = 0X7610FE76;

      static const s32 HEADER_LENGTH = 8;
      static const s32 FOOTER_LENGTH = 8;

      s32 size[2];
      s32 stride;
      bool useBoundaryFillPatterns;

      u16 * data;

      // To enforce alignment, rawDataPointer may be slightly before u16 * data.
      // If the inputted data buffer was from malloc, this is the pointer that
      // should be used to free.
      void * rawDataPointer;

      void initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns);

      void invalidateArray(); // Set all the buffers and sizes to zero, to signal an invalid array

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cv::Mat_<u16> cvMatMirror;
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    private:
      //Array_u16 & operator= (const Array_u16 & rightHandSide); // Not allowed
    }; // class Array_u16

    class Array_s16
    {
    public:
      static s32 ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns);

      static s32 ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns);

      Array_s16();

      // Constructor for a Array_s16, pointing to user-allocated data. If the pointer to *data is not
      // aligned to MEMORY_ALIGNMENT, this Array_s16 will start at the next aligned location.
      // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
      // it may make it hard to convert from OpenCV to Array_s16, though the reverse is trivial.
      Array_s16(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const bool useBoundaryFillPatterns=false);

      // Constructor for a Array_s16, pointing to user-allocated MemoryStack
      Array_s16(const s32 numRows, const s32 numCols, MemoryStack &memory, const bool useBoundaryFillPatterns=false);

      // Pointer to the data, at a given (y,x) location
      inline const s16* Array_s16::Pointer(const s32 index0, const s32 index1) const
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<const s16*>( reinterpret_cast<const char*>(this->data) +
          index1*sizeof(s16) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline s16* Array_s16::Pointer(const s32 index0, const s32 index1)
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<s16*>( reinterpret_cast<char*>(this->data) +
          index1*sizeof(s16) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline const s16* Array_s16::Pointer(const Point_s16 &point) const
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

      // Pointer to the data, at a given (y,x) location
      inline s16* Array_s16::Pointer(const Point_s16 &point)
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      void Show(const char * const windowName, const bool waitForKeypress) const;

      // Returns a templated cv::Mat_ that shares the same buffer with this Array_s16. No data is copied.
      cv::Mat_<s16>& get_CvMat_();
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

      // Print out the contents of this Array_s16
      void Print() const;

      // If the Array_s16 was constructed with the useBoundaryFillPatterns=true, then
      // return if any memory was written out of bounds (via fill patterns at the
      // beginning and end).  If the Array_s16 wasn't constructed with the
      // useBoundaryFillPatterns=true, this method always returns true
      bool IsValid() const;

      // Set every element in the Array_s16 to this value
      // Returns the number of values set
      s32 Set(const s16 value);

      // Parse a space-seperated string, and copy values to this Array_s16.
      // If the string doesn't contain enough elements, the remainder of the Array_s16 will be filled with zeros.
      // Returns the number of values set (not counting extra zeros)
      s32 Set(const char * const values);

      // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
      s32 get_size(s32 dimension) const;

      s32 get_stride() const;

      void* get_rawDataPointer();

      const void* get_rawDataPointer() const;

    protected:
      // Bit-inverse of MemoryStack patterns. The pattern will be put twice at
      // the beginning and end of each line.
      static const u32 FILL_PATTERN_START = 0X5432EF76;
      static const u32 FILL_PATTERN_END = 0X7610FE76;

      static const s32 HEADER_LENGTH = 8;
      static const s32 FOOTER_LENGTH = 8;

      s32 size[2];
      s32 stride;
      bool useBoundaryFillPatterns;

      s16 * data;

      // To enforce alignment, rawDataPointer may be slightly before s16 * data.
      // If the inputted data buffer was from malloc, this is the pointer that
      // should be used to free.
      void * rawDataPointer;

      void initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns);

      void invalidateArray(); // Set all the buffers and sizes to zero, to signal an invalid array

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cv::Mat_<s16> cvMatMirror;
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    private:
      //Array_s16 & operator= (const Array_s16 & rightHandSide); // Not allowed
    }; // class Array_s16

    class Array_u32
    {
    public:
      static s32 ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns);

      static s32 ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns);

      Array_u32();

      // Constructor for a Array_u32, pointing to user-allocated data. If the pointer to *data is not
      // aligned to MEMORY_ALIGNMENT, this Array_u32 will start at the next aligned location.
      // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
      // it may make it hard to convert from OpenCV to Array_u32, though the reverse is trivial.
      Array_u32(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const bool useBoundaryFillPatterns=false);

      // Constructor for a Array_u32, pointing to user-allocated MemoryStack
      Array_u32(const s32 numRows, const s32 numCols, MemoryStack &memory, const bool useBoundaryFillPatterns=false);

      // Pointer to the data, at a given (y,x) location
      inline const u32* Array_u32::Pointer(const s32 index0, const s32 index1) const
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<const u32*>( reinterpret_cast<const char*>(this->data) +
          index1*sizeof(u32) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline u32* Array_u32::Pointer(const s32 index0, const s32 index1)
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) +
          index1*sizeof(u32) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline const u32* Array_u32::Pointer(const Point_s16 &point) const
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

      // Pointer to the data, at a given (y,x) location
      inline u32* Array_u32::Pointer(const Point_s16 &point)
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      void Show(const char * const windowName, const bool waitForKeypress) const;

      // Returns a templated cv::Mat_ that shares the same buffer with this Array_u32. No data is copied.
      cv::Mat_<u32>& get_CvMat_();
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

      // Print out the contents of this Array_u32
      void Print() const;

      // If the Array_u32 was constructed with the useBoundaryFillPatterns=true, then
      // return if any memory was written out of bounds (via fill patterns at the
      // beginning and end).  If the Array_u32 wasn't constructed with the
      // useBoundaryFillPatterns=true, this method always returns true
      bool IsValid() const;

      // Set every element in the Array_u32 to this value
      // Returns the number of values set
      s32 Set(const u32 value);

      // Parse a space-seperated string, and copy values to this Array_u32.
      // If the string doesn't contain enough elements, the remainder of the Array_u32 will be filled with zeros.
      // Returns the number of values set (not counting extra zeros)
      s32 Set(const char * const values);

      // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
      s32 get_size(s32 dimension) const;

      s32 get_stride() const;

      void* get_rawDataPointer();

      const void* get_rawDataPointer() const;

    protected:
      // Bit-inverse of MemoryStack patterns. The pattern will be put twice at
      // the beginning and end of each line.
      static const u32 FILL_PATTERN_START = 0X5432EF76;
      static const u32 FILL_PATTERN_END = 0X7610FE76;

      static const s32 HEADER_LENGTH = 8;
      static const s32 FOOTER_LENGTH = 8;

      s32 size[2];
      s32 stride;
      bool useBoundaryFillPatterns;

      u32 * data;

      // To enforce alignment, rawDataPointer may be slightly before u32 * data.
      // If the inputted data buffer was from malloc, this is the pointer that
      // should be used to free.
      void * rawDataPointer;

      void initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns);

      void invalidateArray(); // Set all the buffers and sizes to zero, to signal an invalid array

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cv::Mat_<u32> cvMatMirror;
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    private:
      //Array_u32 & operator= (const Array_u32 & rightHandSide); // Not allowed
    }; // class Array_u32

    class Array_s32
    {
    public:
      static s32 ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns);

      static s32 ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns);

      Array_s32();

      // Constructor for a Array_s32, pointing to user-allocated data. If the pointer to *data is not
      // aligned to MEMORY_ALIGNMENT, this Array_s32 will start at the next aligned location.
      // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
      // it may make it hard to convert from OpenCV to Array_s32, though the reverse is trivial.
      Array_s32(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const bool useBoundaryFillPatterns=false);

      // Constructor for a Array_s32, pointing to user-allocated MemoryStack
      Array_s32(const s32 numRows, const s32 numCols, MemoryStack &memory, const bool useBoundaryFillPatterns=false);

      // Pointer to the data, at a given (y,x) location
      inline const s32* Array_s32::Pointer(const s32 index0, const s32 index1) const
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<const s32*>( reinterpret_cast<const char*>(this->data) +
          index1*sizeof(s32) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline s32* Array_s32::Pointer(const s32 index0, const s32 index1)
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<s32*>( reinterpret_cast<char*>(this->data) +
          index1*sizeof(s32) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline const s32* Array_s32::Pointer(const Point_s16 &point) const
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

      // Pointer to the data, at a given (y,x) location
      inline s32* Array_s32::Pointer(const Point_s16 &point)
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      void Show(const char * const windowName, const bool waitForKeypress) const;

      // Returns a templated cv::Mat_ that shares the same buffer with this Array_s32. No data is copied.
      cv::Mat_<s32>& get_CvMat_();
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

      // Print out the contents of this Array_s32
      void Print() const;

      // If the Array_s32 was constructed with the useBoundaryFillPatterns=true, then
      // return if any memory was written out of bounds (via fill patterns at the
      // beginning and end).  If the Array_s32 wasn't constructed with the
      // useBoundaryFillPatterns=true, this method always returns true
      bool IsValid() const;

      // Set every element in the Array_s32 to this value
      // Returns the number of values set
      s32 Set(const s32 value);

      // Parse a space-seperated string, and copy values to this Array_s32.
      // If the string doesn't contain enough elements, the remainder of the Array_s32 will be filled with zeros.
      // Returns the number of values set (not counting extra zeros)
      s32 Set(const char * const values);

      // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
      s32 get_size(s32 dimension) const;

      s32 get_stride() const;

      void* get_rawDataPointer();

      const void* get_rawDataPointer() const;

    protected:
      // Bit-inverse of MemoryStack patterns. The pattern will be put twice at
      // the beginning and end of each line.
      static const u32 FILL_PATTERN_START = 0X5432EF76;
      static const u32 FILL_PATTERN_END = 0X7610FE76;

      static const s32 HEADER_LENGTH = 8;
      static const s32 FOOTER_LENGTH = 8;

      s32 size[2];
      s32 stride;
      bool useBoundaryFillPatterns;

      s32 * data;

      // To enforce alignment, rawDataPointer may be slightly before s32 * data.
      // If the inputted data buffer was from malloc, this is the pointer that
      // should be used to free.
      void * rawDataPointer;

      void initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns);

      void invalidateArray(); // Set all the buffers and sizes to zero, to signal an invalid array

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cv::Mat_<s32> cvMatMirror;
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    private:
      //Array_s32 & operator= (const Array_s32 & rightHandSide); // Not allowed
    }; // class Array_s32

    class Array_f32
    {
    public:
      static s32 ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns);

      static s32 ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns);

      Array_f32();

      // Constructor for a Array_f32, pointing to user-allocated data. If the pointer to *data is not
      // aligned to MEMORY_ALIGNMENT, this Array_f32 will start at the next aligned location.
      // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
      // it may make it hard to convert from OpenCV to Array_f32, though the reverse is trivial.
      Array_f32(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const bool useBoundaryFillPatterns=false);

      // Constructor for a Array_f32, pointing to user-allocated MemoryStack
      Array_f32(const s32 numRows, const s32 numCols, MemoryStack &memory, const bool useBoundaryFillPatterns=false);

      // Pointer to the data, at a given (y,x) location
      inline const f32* Array_f32::Pointer(const s32 index0, const s32 index1) const
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<const f32*>( reinterpret_cast<const char*>(this->data) +
          index1*sizeof(f32) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline f32* Array_f32::Pointer(const s32 index0, const s32 index1)
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<f32*>( reinterpret_cast<char*>(this->data) +
          index1*sizeof(f32) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline const f32* Array_f32::Pointer(const Point_s16 &point) const
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

      // Pointer to the data, at a given (y,x) location
      inline f32* Array_f32::Pointer(const Point_s16 &point)
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      void Show(const char * const windowName, const bool waitForKeypress) const;

      // Returns a templated cv::Mat_ that shares the same buffer with this Array_f32. No data is copied.
      cv::Mat_<f32>& get_CvMat_();
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

      // Print out the contents of this Array_f32
      void Print() const;

      // If the Array_f32 was constructed with the useBoundaryFillPatterns=true, then
      // return if any memory was written out of bounds (via fill patterns at the
      // beginning and end).  If the Array_f32 wasn't constructed with the
      // useBoundaryFillPatterns=true, this method always returns true
      bool IsValid() const;

      // Set every element in the Array_f32 to this value
      // Returns the number of values set
      s32 Set(const f32 value);

      // Parse a space-seperated string, and copy values to this Array_f32.
      // If the string doesn't contain enough elements, the remainder of the Array_f32 will be filled with zeros.
      // Returns the number of values set (not counting extra zeros)
      s32 Set(const char * const values);

      // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
      s32 get_size(s32 dimension) const;

      s32 get_stride() const;

      void* get_rawDataPointer();

      const void* get_rawDataPointer() const;

    protected:
      // Bit-inverse of MemoryStack patterns. The pattern will be put twice at
      // the beginning and end of each line.
      static const u32 FILL_PATTERN_START = 0X5432EF76;
      static const u32 FILL_PATTERN_END = 0X7610FE76;

      static const s32 HEADER_LENGTH = 8;
      static const s32 FOOTER_LENGTH = 8;

      s32 size[2];
      s32 stride;
      bool useBoundaryFillPatterns;

      f32 * data;

      // To enforce alignment, rawDataPointer may be slightly before f32 * data.
      // If the inputted data buffer was from malloc, this is the pointer that
      // should be used to free.
      void * rawDataPointer;

      void initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns);

      void invalidateArray(); // Set all the buffers and sizes to zero, to signal an invalid array

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cv::Mat_<f32> cvMatMirror;
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    private:
      //Array_f32 & operator= (const Array_f32 & rightHandSide); // Not allowed
    }; // class Array_f32

    class Array_f64
    {
    public:
      static s32 ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns);

      static s32 ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns);

      Array_f64();

      // Constructor for a Array_f64, pointing to user-allocated data. If the pointer to *data is not
      // aligned to MEMORY_ALIGNMENT, this Array_f64 will start at the next aligned location.
      // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
      // it may make it hard to convert from OpenCV to Array_f64, though the reverse is trivial.
      Array_f64(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const bool useBoundaryFillPatterns=false);

      // Constructor for a Array_f64, pointing to user-allocated MemoryStack
      Array_f64(const s32 numRows, const s32 numCols, MemoryStack &memory, const bool useBoundaryFillPatterns=false);

      // Pointer to the data, at a given (y,x) location
      inline const f64* Array_f64::Pointer(const s32 index0, const s32 index1) const
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<const f64*>( reinterpret_cast<const char*>(this->data) +
          index1*sizeof(f64) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline f64* Array_f64::Pointer(const s32 index0, const s32 index1)
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<f64*>( reinterpret_cast<char*>(this->data) +
          index1*sizeof(f64) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline const f64* Array_f64::Pointer(const Point_s16 &point) const
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

      // Pointer to the data, at a given (y,x) location
      inline f64* Array_f64::Pointer(const Point_s16 &point)
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      void Show(const char * const windowName, const bool waitForKeypress) const;

      // Returns a templated cv::Mat_ that shares the same buffer with this Array_f64. No data is copied.
      cv::Mat_<f64>& get_CvMat_();
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

      // Print out the contents of this Array_f64
      void Print() const;

      // If the Array_f64 was constructed with the useBoundaryFillPatterns=true, then
      // return if any memory was written out of bounds (via fill patterns at the
      // beginning and end).  If the Array_f64 wasn't constructed with the
      // useBoundaryFillPatterns=true, this method always returns true
      bool IsValid() const;

      // Set every element in the Array_f64 to this value
      // Returns the number of values set
      s32 Set(const f64 value);

      // Parse a space-seperated string, and copy values to this Array_f64.
      // If the string doesn't contain enough elements, the remainder of the Array_f64 will be filled with zeros.
      // Returns the number of values set (not counting extra zeros)
      s32 Set(const char * const values);

      // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
      s32 get_size(s32 dimension) const;

      s32 get_stride() const;

      void* get_rawDataPointer();

      const void* get_rawDataPointer() const;

    protected:
      // Bit-inverse of MemoryStack patterns. The pattern will be put twice at
      // the beginning and end of each line.
      static const u32 FILL_PATTERN_START = 0X5432EF76;
      static const u32 FILL_PATTERN_END = 0X7610FE76;

      static const s32 HEADER_LENGTH = 8;
      static const s32 FOOTER_LENGTH = 8;

      s32 size[2];
      s32 stride;
      bool useBoundaryFillPatterns;

      f64 * data;

      // To enforce alignment, rawDataPointer may be slightly before f64 * data.
      // If the inputted data buffer was from malloc, this is the pointer that
      // should be used to free.
      void * rawDataPointer;

      void initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns);

      void invalidateArray(); // Set all the buffers and sizes to zero, to signal an invalid array

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cv::Mat_<f64> cvMatMirror;
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    private:
      //Array_f64 & operator= (const Array_f64 & rightHandSide); // Not allowed
    }; // class Array_f64

    class Array_Point_s16
    {
    public:
      static s32 ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns);

      static s32 ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns);

      Array_Point_s16();

      // Constructor for a Array_Point_s16, pointing to user-allocated data. If the pointer to *data is not
      // aligned to MEMORY_ALIGNMENT, this Array_Point_s16 will start at the next aligned location.
      // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
      // it may make it hard to convert from OpenCV to Array_Point_s16, though the reverse is trivial.
      Array_Point_s16(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const bool useBoundaryFillPatterns=false);

      // Constructor for a Array_Point_s16, pointing to user-allocated MemoryStack
      Array_Point_s16(const s32 numRows, const s32 numCols, MemoryStack &memory, const bool useBoundaryFillPatterns=false);

      // Pointer to the data, at a given (y,x) location
      inline const Point_s16* Array_Point_s16::Pointer(const s32 index0, const s32 index1) const
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<const Point_s16*>( reinterpret_cast<const char*>(this->data) +
          index1*sizeof(Point_s16) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline Point_s16* Array_Point_s16::Pointer(const s32 index0, const s32 index1)
      {
        assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
          this->rawDataPointer != NULL && this->data != NULL);

        return reinterpret_cast<Point_s16*>( reinterpret_cast<char*>(this->data) +
          index1*sizeof(Point_s16) + index0*stride );
      }

      // Pointer to the data, at a given (y,x) location
      inline const Point_s16* Array_Point_s16::Pointer(const Point_s16 &point) const
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

      // Pointer to the data, at a given (y,x) location
      inline Point_s16* Array_Point_s16::Pointer(const Point_s16 &point)
      {
        return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      void Show(const char * const windowName, const bool waitForKeypress) const;

      // Returns a templated cv::Mat_ that shares the same buffer with this Array_Point_s16. No data is copied.
      cv::Mat_<Point_s16>& get_CvMat_();
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

      // Print out the contents of this Array_Point_s16
      void Print() const;

      // If the Array_Point_s16 was constructed with the useBoundaryFillPatterns=true, then
      // return if any memory was written out of bounds (via fill patterns at the
      // beginning and end).  If the Array_Point_s16 wasn't constructed with the
      // useBoundaryFillPatterns=true, this method always returns true
      bool IsValid() const;

      // Set every element in the Array_Point_s16 to this value
      // Returns the number of values set
      s32 Set(const Point_s16 value);

      // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
      s32 get_size(s32 dimension) const;

      s32 get_stride() const;

      void* get_rawDataPointer();

      const void* get_rawDataPointer() const;

    protected:
      // Bit-inverse of MemoryStack patterns. The pattern will be put twice at
      // the beginning and end of each line.
      static const u32 FILL_PATTERN_START = 0X5432EF76;
      static const u32 FILL_PATTERN_END = 0X7610FE76;

      static const s32 HEADER_LENGTH = 8;
      static const s32 FOOTER_LENGTH = 8;

      s32 size[2];
      s32 stride;
      bool useBoundaryFillPatterns;

      Point_s16 * data;

      // To enforce alignment, rawDataPointer may be slightly before Point_s16 * data.
      // If the inputted data buffer was from malloc, this is the pointer that
      // should be used to free.
      void * rawDataPointer;

      void initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns);

      void invalidateArray(); // Set all the buffers and sizes to zero, to signal an invalid array

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cv::Mat_<Point_s16> cvMatMirror;
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    private:
      //Array_Point_s16 & operator= (const Array_Point_s16 & rightHandSide); // Not allowed
    }; // class Array_Point_s16
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_ARRAY2D_H_
