//#include "anki/embeddedCommon.h"

//#include <iostream>

#include "anki/embeddedCommon/array2d.h"

namespace Anki
{
  namespace Embedded
  {
    s32 Array_u8::ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0);
      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? (HEADER_LENGTH+FOOTER_LENGTH) : 0);
      return static_cast<s32>(RoundUp<size_t>(sizeof(u8)*numCols, MEMORY_ALIGNMENT)) + extraBoundaryPatternBytes;
    }

    s32 Array_u8::ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0);
      return numRows * Array_u8::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
    }

    Array_u8::Array_u8()
    {
      invalidateArray();
    }

    // Constructor for a Array_u8, pointing to user-allocated data. If the pointer to *data is not
    // aligned to MEMORY_ALIGNMENT, this Array_u8 will start at the next aligned location.
    // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
    // it may make it hard to convert from OpenCV to Array_u8, though the reverse is trivial.
    Array_u8::Array_u8(s32 numRows, s32 numCols, void * data, s32 dataLength, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      initialize(numRows,
        numCols,
        data,
        dataLength,
        useBoundaryFillPatterns);
    }

    Array_u8::Array_u8(s32 numRows, s32 numCols, MemoryStack &memory, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0);

      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? static_cast<s32>(MEMORY_ALIGNMENT) : 0);
      const s32 numBytesRequested = numRows * this->stride + extraBoundaryPatternBytes;
      s32 numBytesAllocated = 0;

      void * allocatedBuffer = memory.Allocate(numBytesRequested, &numBytesAllocated);

      initialize(numRows,
        numCols,
        reinterpret_cast<u8*>(allocatedBuffer),
        numBytesAllocated,
        useBoundaryFillPatterns);
    }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    void Array_u8::Show(const char * const windowName, const bool waitForKeypress) const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      cv::imshow(windowName, cvMatMirror);
      if(waitForKeypress) {
        cv::waitKey();
      }
    }

    // Returns a templated cv::Mat_ that shares the same buffer with this Array_u8. No data is copied.
    cv::Mat_<u8>& Array_u8::get_CvMat_()
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      return cvMatMirror;
    }
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    // Print out the contents of this Array_u8
    void Array_u8::Print() const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        const u8 * rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          //std::cout << rowPointer[x] << " ";
          printf("%d ", rowPointer[x]); // TODO: make general
        }
        // std::cout << "\n";
        printf("\n");
      }
    }

    // If the Array_u8 was constructed with the useBoundaryFillPatterns=true, then
    // return if any memory was written out of bounds (via fill patterns at the
    // beginning and end).  If the Array_u8 wasn't constructed with the
    // useBoundaryFillPatterns=true, this method always returns true
    bool Array_u8::IsValid() const
    {
      if(this->rawDataPointer == NULL || this->data == NULL) {
        return false;
      }

      if(size[0] < 1 || size[1] < 1) {
        return false;
      }

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1],false);

        for(s32 y=0; y<size[0]; y++) {
          if((reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0]) != FILL_PATTERN_END ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1]) != FILL_PATTERN_END) {
              return false;
          }
        }

        return true;
      } else { // if(useBoundaryFillPatterns) {
        return true; // Technically, we don't know if the Array2d is valid. But we don't know it's NOT valid, so just return true.
      } // if(useBoundaryFillPatterns) { ... else
    }

    // Set every element in the Array_u8 to this value
    // Returns the number of values set
    s32 Array_u8::Set(const u8 value)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        u8 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          rowPointer[x] = value;
        }
      }

      return size[0]*size[1];
    }

    // Parse a space-seperated string, and copy values to this Array_u8.
    // If the string doesn't contain enough elements, the remainder of the Array_u8 will be filled with zeros.
    // Returns the number of values set (not counting extra zeros)
    s32 Array_u8::Set(const char * const values)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      s32 numValuesSet = 0;

      const char * startPointer = values;
      char * endPointer = NULL;

      for(s32 y=0; y<size[0]; y++) {
        u8 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          u8 value = static_cast<u8>(strtol(startPointer, &endPointer, 10));
          if(startPointer != endPointer) {
            rowPointer[x] = value;
            numValuesSet++;
          } else {
            rowPointer[x] = 0;
          }
          startPointer = endPointer;
        }
      }

      return numValuesSet;
    }

    // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
    s32 Array_u8::get_size(s32 dimension) const
    {
      assert(dimension >= 0 && this->rawDataPointer != NULL && this->data != NULL);

      if(dimension > 1 || dimension < 0)
        return 0;

      return size[dimension];
    }

    s32 Array_u8::get_stride() const
    {
      return stride;
    }

    void* Array_u8::get_rawDataPointer()
    {
      return rawDataPointer;
    }

    const void* Array_u8::get_rawDataPointer() const
    {
      return rawDataPointer;
    }

    void Array_u8::initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      this->useBoundaryFillPatterns = useBoundaryFillPatterns;

      if(!rawData) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "input data buffer is NULL");
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->rawDataPointer = rawData;

      const size_t extraBoundaryPatternBytes = useBoundaryFillPatterns ? static_cast<size_t>(HEADER_LENGTH) : 0;
      const s32 extraAlignmentBytes = static_cast<s32>(RoundUp<size_t>(reinterpret_cast<size_t>(rawData)+extraBoundaryPatternBytes, MEMORY_ALIGNMENT) - extraBoundaryPatternBytes - reinterpret_cast<size_t>(rawData));
      const s32 requiredBytes = ComputeRequiredStride(numCols,useBoundaryFillPatterns)*numRows + extraAlignmentBytes;

      if(requiredBytes > dataLength) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "Input data buffer is not large enough. %d bytes is required.", requiredBytes);
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->size[0] = numRows;
      this->size[1] = numCols;

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1], false);
        this->data = reinterpret_cast<u8*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes + HEADER_LENGTH );
        for(s32 y=0; y<size[0]; y++) {
          // Add the fill patterns just before the data on each line
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0] = FILL_PATTERN_START;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1] = FILL_PATTERN_START;

          // And also just after the data (including normal byte-alignment padding)
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0] = FILL_PATTERN_END;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1] = FILL_PATTERN_END;
        }
      } else {
        this->data = reinterpret_cast<u8*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes );
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cvMatMirror = cv::Mat_<u8>(size[0], size[1], data, stride);
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    } // Array_u8::initialize()

    // Set all the buffers and sizes to zero, to signal an invalid array
    void Array_u8::invalidateArray()
    {
      this->size[0] = 0;
      this->size[1] = 0;
      this->stride = 0;
      this->data = NULL;
      this->rawDataPointer = NULL;
    }

    s32 Array_s8::ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0);
      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? (HEADER_LENGTH+FOOTER_LENGTH) : 0);
      return static_cast<s32>(RoundUp<size_t>(sizeof(s8)*numCols, MEMORY_ALIGNMENT)) + extraBoundaryPatternBytes;
    }

    s32 Array_s8::ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0);
      return numRows * Array_s8::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
    }

    Array_s8::Array_s8()
    {
      invalidateArray();
    }

    // Constructor for a Array_s8, pointing to user-allocated data. If the pointer to *data is not
    // aligned to MEMORY_ALIGNMENT, this Array_s8 will start at the next aligned location.
    // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
    // it may make it hard to convert from OpenCV to Array_s8, though the reverse is trivial.
    Array_s8::Array_s8(s32 numRows, s32 numCols, void * data, s32 dataLength, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      initialize(numRows,
        numCols,
        data,
        dataLength,
        useBoundaryFillPatterns);
    }

    Array_s8::Array_s8(s32 numRows, s32 numCols, MemoryStack &memory, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0);

      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? static_cast<s32>(MEMORY_ALIGNMENT) : 0);
      const s32 numBytesRequested = numRows * this->stride + extraBoundaryPatternBytes;
      s32 numBytesAllocated = 0;

      void * allocatedBuffer = memory.Allocate(numBytesRequested, &numBytesAllocated);

      initialize(numRows,
        numCols,
        reinterpret_cast<s8*>(allocatedBuffer),
        numBytesAllocated,
        useBoundaryFillPatterns);
    }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    void Array_s8::Show(const char * const windowName, const bool waitForKeypress) const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      cv::imshow(windowName, cvMatMirror);
      if(waitForKeypress) {
        cv::waitKey();
      }
    }

    // Returns a templated cv::Mat_ that shares the same buffer with this Array_s8. No data is copied.
    cv::Mat_<s8>& Array_s8::get_CvMat_()
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      return cvMatMirror;
    }
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    // Print out the contents of this Array_s8
    void Array_s8::Print() const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        const s8 * rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          //std::cout << rowPointer[x] << " ";
          printf("%d ", rowPointer[x]); // TODO: make general
        }
        // std::cout << "\n";
        printf("\n");
      }
    }

    // If the Array_s8 was constructed with the useBoundaryFillPatterns=true, then
    // return if any memory was written out of bounds (via fill patterns at the
    // beginning and end).  If the Array_s8 wasn't constructed with the
    // useBoundaryFillPatterns=true, this method always returns true
    bool Array_s8::IsValid() const
    {
      if(this->rawDataPointer == NULL || this->data == NULL) {
        return false;
      }

      if(size[0] < 1 || size[1] < 1) {
        return false;
      }

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1],false);

        for(s32 y=0; y<size[0]; y++) {
          if((reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0]) != FILL_PATTERN_END ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1]) != FILL_PATTERN_END) {
              return false;
          }
        }

        return true;
      } else { // if(useBoundaryFillPatterns) {
        return true; // Technically, we don't know if the Array2d is valid. But we don't know it's NOT valid, so just return true.
      } // if(useBoundaryFillPatterns) { ... else
    }

    // Set every element in the Array_s8 to this value
    // Returns the number of values set
    s32 Array_s8::Set(const s8 value)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        s8 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          rowPointer[x] = value;
        }
      }

      return size[0]*size[1];
    }

    // Parse a space-seperated string, and copy values to this Array_s8.
    // If the string doesn't contain enough elements, the remainder of the Array_s8 will be filled with zeros.
    // Returns the number of values set (not counting extra zeros)
    s32 Array_s8::Set(const char * const values)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      s32 numValuesSet = 0;

      const char * startPointer = values;
      char * endPointer = NULL;

      for(s32 y=0; y<size[0]; y++) {
        s8 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          s8 value = static_cast<s8>(strtol(startPointer, &endPointer, 10));
          if(startPointer != endPointer) {
            rowPointer[x] = value;
            numValuesSet++;
          } else {
            rowPointer[x] = 0;
          }
          startPointer = endPointer;
        }
      }

      return numValuesSet;
    }

    // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
    s32 Array_s8::get_size(s32 dimension) const
    {
      assert(dimension >= 0 && this->rawDataPointer != NULL && this->data != NULL);

      if(dimension > 1 || dimension < 0)
        return 0;

      return size[dimension];
    }

    s32 Array_s8::get_stride() const
    {
      return stride;
    }

    void* Array_s8::get_rawDataPointer()
    {
      return rawDataPointer;
    }

    const void* Array_s8::get_rawDataPointer() const
    {
      return rawDataPointer;
    }

    void Array_s8::initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      this->useBoundaryFillPatterns = useBoundaryFillPatterns;

      if(!rawData) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "input data buffer is NULL");
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->rawDataPointer = rawData;

      const size_t extraBoundaryPatternBytes = useBoundaryFillPatterns ? static_cast<size_t>(HEADER_LENGTH) : 0;
      const s32 extraAlignmentBytes = static_cast<s32>(RoundUp<size_t>(reinterpret_cast<size_t>(rawData)+extraBoundaryPatternBytes, MEMORY_ALIGNMENT) - extraBoundaryPatternBytes - reinterpret_cast<size_t>(rawData));
      const s32 requiredBytes = ComputeRequiredStride(numCols,useBoundaryFillPatterns)*numRows + extraAlignmentBytes;

      if(requiredBytes > dataLength) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "Input data buffer is not large enough. %d bytes is required.", requiredBytes);
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->size[0] = numRows;
      this->size[1] = numCols;

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1], false);
        this->data = reinterpret_cast<s8*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes + HEADER_LENGTH );
        for(s32 y=0; y<size[0]; y++) {
          // Add the fill patterns just before the data on each line
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0] = FILL_PATTERN_START;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1] = FILL_PATTERN_START;

          // And also just after the data (including normal byte-alignment padding)
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0] = FILL_PATTERN_END;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1] = FILL_PATTERN_END;
        }
      } else {
        this->data = reinterpret_cast<s8*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes );
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cvMatMirror = cv::Mat_<s8>(size[0], size[1], data, stride);
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    } // Array_s8::initialize()

    // Set all the buffers and sizes to zero, to signal an invalid array
    void Array_s8::invalidateArray()
    {
      this->size[0] = 0;
      this->size[1] = 0;
      this->stride = 0;
      this->data = NULL;
      this->rawDataPointer = NULL;
    }

    s32 Array_u16::ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0);
      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? (HEADER_LENGTH+FOOTER_LENGTH) : 0);
      return static_cast<s32>(RoundUp<size_t>(sizeof(u16)*numCols, MEMORY_ALIGNMENT)) + extraBoundaryPatternBytes;
    }

    s32 Array_u16::ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0);
      return numRows * Array_u16::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
    }

    Array_u16::Array_u16()
    {
      invalidateArray();
    }

    // Constructor for a Array_u16, pointing to user-allocated data. If the pointer to *data is not
    // aligned to MEMORY_ALIGNMENT, this Array_u16 will start at the next aligned location.
    // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
    // it may make it hard to convert from OpenCV to Array_u16, though the reverse is trivial.
    Array_u16::Array_u16(s32 numRows, s32 numCols, void * data, s32 dataLength, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      initialize(numRows,
        numCols,
        data,
        dataLength,
        useBoundaryFillPatterns);
    }

    Array_u16::Array_u16(s32 numRows, s32 numCols, MemoryStack &memory, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0);

      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? static_cast<s32>(MEMORY_ALIGNMENT) : 0);
      const s32 numBytesRequested = numRows * this->stride + extraBoundaryPatternBytes;
      s32 numBytesAllocated = 0;

      void * allocatedBuffer = memory.Allocate(numBytesRequested, &numBytesAllocated);

      initialize(numRows,
        numCols,
        reinterpret_cast<u16*>(allocatedBuffer),
        numBytesAllocated,
        useBoundaryFillPatterns);
    }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    void Array_u16::Show(const char * const windowName, const bool waitForKeypress) const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      cv::imshow(windowName, cvMatMirror);
      if(waitForKeypress) {
        cv::waitKey();
      }
    }

    // Returns a templated cv::Mat_ that shares the same buffer with this Array_u16. No data is copied.
    cv::Mat_<u16>& Array_u16::get_CvMat_()
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      return cvMatMirror;
    }
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    // Print out the contents of this Array_u16
    void Array_u16::Print() const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        const u16 * rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          //std::cout << rowPointer[x] << " ";
          printf("%d ", rowPointer[x]); // TODO: make general
        }
        // std::cout << "\n";
        printf("\n");
      }
    }

    // If the Array_u16 was constructed with the useBoundaryFillPatterns=true, then
    // return if any memory was written out of bounds (via fill patterns at the
    // beginning and end).  If the Array_u16 wasn't constructed with the
    // useBoundaryFillPatterns=true, this method always returns true
    bool Array_u16::IsValid() const
    {
      if(this->rawDataPointer == NULL || this->data == NULL) {
        return false;
      }

      if(size[0] < 1 || size[1] < 1) {
        return false;
      }

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1],false);

        for(s32 y=0; y<size[0]; y++) {
          if((reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0]) != FILL_PATTERN_END ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1]) != FILL_PATTERN_END) {
              return false;
          }
        }

        return true;
      } else { // if(useBoundaryFillPatterns) {
        return true; // Technically, we don't know if the Array2d is valid. But we don't know it's NOT valid, so just return true.
      } // if(useBoundaryFillPatterns) { ... else
    }

    // Set every element in the Array_u16 to this value
    // Returns the number of values set
    s32 Array_u16::Set(const u16 value)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        u16 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          rowPointer[x] = value;
        }
      }

      return size[0]*size[1];
    }

    // Parse a space-seperated string, and copy values to this Array_u16.
    // If the string doesn't contain enough elements, the remainder of the Array_u16 will be filled with zeros.
    // Returns the number of values set (not counting extra zeros)
    s32 Array_u16::Set(const char * const values)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      s32 numValuesSet = 0;

      const char * startPointer = values;
      char * endPointer = NULL;

      for(s32 y=0; y<size[0]; y++) {
        u16 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          u16 value = static_cast<u16>(strtol(startPointer, &endPointer, 10));
          if(startPointer != endPointer) {
            rowPointer[x] = value;
            numValuesSet++;
          } else {
            rowPointer[x] = 0;
          }
          startPointer = endPointer;
        }
      }

      return numValuesSet;
    }

    // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
    s32 Array_u16::get_size(s32 dimension) const
    {
      assert(dimension >= 0 && this->rawDataPointer != NULL && this->data != NULL);

      if(dimension > 1 || dimension < 0)
        return 0;

      return size[dimension];
    }

    s32 Array_u16::get_stride() const
    {
      return stride;
    }

    void* Array_u16::get_rawDataPointer()
    {
      return rawDataPointer;
    }

    const void* Array_u16::get_rawDataPointer() const
    {
      return rawDataPointer;
    }

    void Array_u16::initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      this->useBoundaryFillPatterns = useBoundaryFillPatterns;

      if(!rawData) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "input data buffer is NULL");
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->rawDataPointer = rawData;

      const size_t extraBoundaryPatternBytes = useBoundaryFillPatterns ? static_cast<size_t>(HEADER_LENGTH) : 0;
      const s32 extraAlignmentBytes = static_cast<s32>(RoundUp<size_t>(reinterpret_cast<size_t>(rawData)+extraBoundaryPatternBytes, MEMORY_ALIGNMENT) - extraBoundaryPatternBytes - reinterpret_cast<size_t>(rawData));
      const s32 requiredBytes = ComputeRequiredStride(numCols,useBoundaryFillPatterns)*numRows + extraAlignmentBytes;

      if(requiredBytes > dataLength) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "Input data buffer is not large enough. %d bytes is required.", requiredBytes);
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->size[0] = numRows;
      this->size[1] = numCols;

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1], false);
        this->data = reinterpret_cast<u16*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes + HEADER_LENGTH );
        for(s32 y=0; y<size[0]; y++) {
          // Add the fill patterns just before the data on each line
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0] = FILL_PATTERN_START;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1] = FILL_PATTERN_START;

          // And also just after the data (including normal byte-alignment padding)
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0] = FILL_PATTERN_END;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1] = FILL_PATTERN_END;
        }
      } else {
        this->data = reinterpret_cast<u16*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes );
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cvMatMirror = cv::Mat_<u16>(size[0], size[1], data, stride);
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    } // Array_u16::initialize()

    // Set all the buffers and sizes to zero, to signal an invalid array
    void Array_u16::invalidateArray()
    {
      this->size[0] = 0;
      this->size[1] = 0;
      this->stride = 0;
      this->data = NULL;
      this->rawDataPointer = NULL;
    }

    s32 Array_s16::ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0);
      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? (HEADER_LENGTH+FOOTER_LENGTH) : 0);
      return static_cast<s32>(RoundUp<size_t>(sizeof(s16)*numCols, MEMORY_ALIGNMENT)) + extraBoundaryPatternBytes;
    }

    s32 Array_s16::ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0);
      return numRows * Array_s16::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
    }

    Array_s16::Array_s16()
    {
      invalidateArray();
    }

    // Constructor for a Array_s16, pointing to user-allocated data. If the pointer to *data is not
    // aligned to MEMORY_ALIGNMENT, this Array_s16 will start at the next aligned location.
    // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
    // it may make it hard to convert from OpenCV to Array_s16, though the reverse is trivial.
    Array_s16::Array_s16(s32 numRows, s32 numCols, void * data, s32 dataLength, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      initialize(numRows,
        numCols,
        data,
        dataLength,
        useBoundaryFillPatterns);
    }

    Array_s16::Array_s16(s32 numRows, s32 numCols, MemoryStack &memory, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0);

      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? static_cast<s32>(MEMORY_ALIGNMENT) : 0);
      const s32 numBytesRequested = numRows * this->stride + extraBoundaryPatternBytes;
      s32 numBytesAllocated = 0;

      void * allocatedBuffer = memory.Allocate(numBytesRequested, &numBytesAllocated);

      initialize(numRows,
        numCols,
        reinterpret_cast<s16*>(allocatedBuffer),
        numBytesAllocated,
        useBoundaryFillPatterns);
    }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    void Array_s16::Show(const char * const windowName, const bool waitForKeypress) const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      cv::imshow(windowName, cvMatMirror);
      if(waitForKeypress) {
        cv::waitKey();
      }
    }

    // Returns a templated cv::Mat_ that shares the same buffer with this Array_s16. No data is copied.
    cv::Mat_<s16>& Array_s16::get_CvMat_()
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      return cvMatMirror;
    }
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    // Print out the contents of this Array_s16
    void Array_s16::Print() const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        const s16 * rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          //std::cout << rowPointer[x] << " ";
          printf("%d ", rowPointer[x]); // TODO: make general
        }
        // std::cout << "\n";
        printf("\n");
      }
    }

    // If the Array_s16 was constructed with the useBoundaryFillPatterns=true, then
    // return if any memory was written out of bounds (via fill patterns at the
    // beginning and end).  If the Array_s16 wasn't constructed with the
    // useBoundaryFillPatterns=true, this method always returns true
    bool Array_s16::IsValid() const
    {
      if(this->rawDataPointer == NULL || this->data == NULL) {
        return false;
      }

      if(size[0] < 1 || size[1] < 1) {
        return false;
      }

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1],false);

        for(s32 y=0; y<size[0]; y++) {
          if((reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0]) != FILL_PATTERN_END ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1]) != FILL_PATTERN_END) {
              return false;
          }
        }

        return true;
      } else { // if(useBoundaryFillPatterns) {
        return true; // Technically, we don't know if the Array2d is valid. But we don't know it's NOT valid, so just return true.
      } // if(useBoundaryFillPatterns) { ... else
    }

    // Set every element in the Array_s16 to this value
    // Returns the number of values set
    s32 Array_s16::Set(const s16 value)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        s16 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          rowPointer[x] = value;
        }
      }

      return size[0]*size[1];
    }

    // Parse a space-seperated string, and copy values to this Array_s16.
    // If the string doesn't contain enough elements, the remainder of the Array_s16 will be filled with zeros.
    // Returns the number of values set (not counting extra zeros)
    s32 Array_s16::Set(const char * const values)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      s32 numValuesSet = 0;

      const char * startPointer = values;
      char * endPointer = NULL;

      for(s32 y=0; y<size[0]; y++) {
        s16 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          s16 value = static_cast<s16>(strtol(startPointer, &endPointer, 10));
          if(startPointer != endPointer) {
            rowPointer[x] = value;
            numValuesSet++;
          } else {
            rowPointer[x] = 0;
          }
          startPointer = endPointer;
        }
      }

      return numValuesSet;
    }

    // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
    s32 Array_s16::get_size(s32 dimension) const
    {
      assert(dimension >= 0 && this->rawDataPointer != NULL && this->data != NULL);

      if(dimension > 1 || dimension < 0)
        return 0;

      return size[dimension];
    }

    s32 Array_s16::get_stride() const
    {
      return stride;
    }

    void* Array_s16::get_rawDataPointer()
    {
      return rawDataPointer;
    }

    const void* Array_s16::get_rawDataPointer() const
    {
      return rawDataPointer;
    }

    void Array_s16::initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      this->useBoundaryFillPatterns = useBoundaryFillPatterns;

      if(!rawData) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "input data buffer is NULL");
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->rawDataPointer = rawData;

      const size_t extraBoundaryPatternBytes = useBoundaryFillPatterns ? static_cast<size_t>(HEADER_LENGTH) : 0;
      const s32 extraAlignmentBytes = static_cast<s32>(RoundUp<size_t>(reinterpret_cast<size_t>(rawData)+extraBoundaryPatternBytes, MEMORY_ALIGNMENT) - extraBoundaryPatternBytes - reinterpret_cast<size_t>(rawData));
      const s32 requiredBytes = ComputeRequiredStride(numCols,useBoundaryFillPatterns)*numRows + extraAlignmentBytes;

      if(requiredBytes > dataLength) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "Input data buffer is not large enough. %d bytes is required.", requiredBytes);
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->size[0] = numRows;
      this->size[1] = numCols;

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1], false);
        this->data = reinterpret_cast<s16*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes + HEADER_LENGTH );
        for(s32 y=0; y<size[0]; y++) {
          // Add the fill patterns just before the data on each line
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0] = FILL_PATTERN_START;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1] = FILL_PATTERN_START;

          // And also just after the data (including normal byte-alignment padding)
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0] = FILL_PATTERN_END;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1] = FILL_PATTERN_END;
        }
      } else {
        this->data = reinterpret_cast<s16*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes );
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cvMatMirror = cv::Mat_<s16>(size[0], size[1], data, stride);
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    } // Array_s16::initialize()

    // Set all the buffers and sizes to zero, to signal an invalid array
    void Array_s16::invalidateArray()
    {
      this->size[0] = 0;
      this->size[1] = 0;
      this->stride = 0;
      this->data = NULL;
      this->rawDataPointer = NULL;
    }

    s32 Array_u32::ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0);
      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? (HEADER_LENGTH+FOOTER_LENGTH) : 0);
      return static_cast<s32>(RoundUp<size_t>(sizeof(u32)*numCols, MEMORY_ALIGNMENT)) + extraBoundaryPatternBytes;
    }

    s32 Array_u32::ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0);
      return numRows * Array_u32::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
    }

    Array_u32::Array_u32()
    {
      invalidateArray();
    }

    // Constructor for a Array_u32, pointing to user-allocated data. If the pointer to *data is not
    // aligned to MEMORY_ALIGNMENT, this Array_u32 will start at the next aligned location.
    // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
    // it may make it hard to convert from OpenCV to Array_u32, though the reverse is trivial.
    Array_u32::Array_u32(s32 numRows, s32 numCols, void * data, s32 dataLength, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      initialize(numRows,
        numCols,
        data,
        dataLength,
        useBoundaryFillPatterns);
    }

    Array_u32::Array_u32(s32 numRows, s32 numCols, MemoryStack &memory, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0);

      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? static_cast<s32>(MEMORY_ALIGNMENT) : 0);
      const s32 numBytesRequested = numRows * this->stride + extraBoundaryPatternBytes;
      s32 numBytesAllocated = 0;

      void * allocatedBuffer = memory.Allocate(numBytesRequested, &numBytesAllocated);

      initialize(numRows,
        numCols,
        reinterpret_cast<u32*>(allocatedBuffer),
        numBytesAllocated,
        useBoundaryFillPatterns);
    }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    void Array_u32::Show(const char * const windowName, const bool waitForKeypress) const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      cv::imshow(windowName, cvMatMirror);
      if(waitForKeypress) {
        cv::waitKey();
      }
    }

    // Returns a templated cv::Mat_ that shares the same buffer with this Array_u32. No data is copied.
    cv::Mat_<u32>& Array_u32::get_CvMat_()
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      return cvMatMirror;
    }
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    // Print out the contents of this Array_u32
    void Array_u32::Print() const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        const u32 * rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          //std::cout << rowPointer[x] << " ";
          printf("%d ", rowPointer[x]); // TODO: make general
        }
        // std::cout << "\n";
        printf("\n");
      }
    }

    // If the Array_u32 was constructed with the useBoundaryFillPatterns=true, then
    // return if any memory was written out of bounds (via fill patterns at the
    // beginning and end).  If the Array_u32 wasn't constructed with the
    // useBoundaryFillPatterns=true, this method always returns true
    bool Array_u32::IsValid() const
    {
      if(this->rawDataPointer == NULL || this->data == NULL) {
        return false;
      }

      if(size[0] < 1 || size[1] < 1) {
        return false;
      }

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1],false);

        for(s32 y=0; y<size[0]; y++) {
          if((reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0]) != FILL_PATTERN_END ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1]) != FILL_PATTERN_END) {
              return false;
          }
        }

        return true;
      } else { // if(useBoundaryFillPatterns) {
        return true; // Technically, we don't know if the Array2d is valid. But we don't know it's NOT valid, so just return true.
      } // if(useBoundaryFillPatterns) { ... else
    }

    // Set every element in the Array_u32 to this value
    // Returns the number of values set
    s32 Array_u32::Set(const u32 value)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        u32 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          rowPointer[x] = value;
        }
      }

      return size[0]*size[1];
    }

    // Parse a space-seperated string, and copy values to this Array_u32.
    // If the string doesn't contain enough elements, the remainder of the Array_u32 will be filled with zeros.
    // Returns the number of values set (not counting extra zeros)
    s32 Array_u32::Set(const char * const values)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      s32 numValuesSet = 0;

      const char * startPointer = values;
      char * endPointer = NULL;

      for(s32 y=0; y<size[0]; y++) {
        u32 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          u32 value = static_cast<u32>(strtol(startPointer, &endPointer, 10));
          if(startPointer != endPointer) {
            rowPointer[x] = value;
            numValuesSet++;
          } else {
            rowPointer[x] = 0;
          }
          startPointer = endPointer;
        }
      }

      return numValuesSet;
    }

    // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
    s32 Array_u32::get_size(s32 dimension) const
    {
      assert(dimension >= 0 && this->rawDataPointer != NULL && this->data != NULL);

      if(dimension > 1 || dimension < 0)
        return 0;

      return size[dimension];
    }

    s32 Array_u32::get_stride() const
    {
      return stride;
    }

    void* Array_u32::get_rawDataPointer()
    {
      return rawDataPointer;
    }

    const void* Array_u32::get_rawDataPointer() const
    {
      return rawDataPointer;
    }

    void Array_u32::initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      this->useBoundaryFillPatterns = useBoundaryFillPatterns;

      if(!rawData) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "input data buffer is NULL");
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->rawDataPointer = rawData;

      const size_t extraBoundaryPatternBytes = useBoundaryFillPatterns ? static_cast<size_t>(HEADER_LENGTH) : 0;
      const s32 extraAlignmentBytes = static_cast<s32>(RoundUp<size_t>(reinterpret_cast<size_t>(rawData)+extraBoundaryPatternBytes, MEMORY_ALIGNMENT) - extraBoundaryPatternBytes - reinterpret_cast<size_t>(rawData));
      const s32 requiredBytes = ComputeRequiredStride(numCols,useBoundaryFillPatterns)*numRows + extraAlignmentBytes;

      if(requiredBytes > dataLength) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "Input data buffer is not large enough. %d bytes is required.", requiredBytes);
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->size[0] = numRows;
      this->size[1] = numCols;

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1], false);
        this->data = reinterpret_cast<u32*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes + HEADER_LENGTH );
        for(s32 y=0; y<size[0]; y++) {
          // Add the fill patterns just before the data on each line
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0] = FILL_PATTERN_START;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1] = FILL_PATTERN_START;

          // And also just after the data (including normal byte-alignment padding)
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0] = FILL_PATTERN_END;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1] = FILL_PATTERN_END;
        }
      } else {
        this->data = reinterpret_cast<u32*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes );
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cvMatMirror = cv::Mat_<u32>(size[0], size[1], data, stride);
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    } // Array_u32::initialize()

    // Set all the buffers and sizes to zero, to signal an invalid array
    void Array_u32::invalidateArray()
    {
      this->size[0] = 0;
      this->size[1] = 0;
      this->stride = 0;
      this->data = NULL;
      this->rawDataPointer = NULL;
    }

    s32 Array_s32::ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0);
      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? (HEADER_LENGTH+FOOTER_LENGTH) : 0);
      return static_cast<s32>(RoundUp<size_t>(sizeof(s32)*numCols, MEMORY_ALIGNMENT)) + extraBoundaryPatternBytes;
    }

    s32 Array_s32::ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0);
      return numRows * Array_s32::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
    }

    Array_s32::Array_s32()
    {
      invalidateArray();
    }

    // Constructor for a Array_s32, pointing to user-allocated data. If the pointer to *data is not
    // aligned to MEMORY_ALIGNMENT, this Array_s32 will start at the next aligned location.
    // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
    // it may make it hard to convert from OpenCV to Array_s32, though the reverse is trivial.
    Array_s32::Array_s32(s32 numRows, s32 numCols, void * data, s32 dataLength, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      initialize(numRows,
        numCols,
        data,
        dataLength,
        useBoundaryFillPatterns);
    }

    Array_s32::Array_s32(s32 numRows, s32 numCols, MemoryStack &memory, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0);

      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? static_cast<s32>(MEMORY_ALIGNMENT) : 0);
      const s32 numBytesRequested = numRows * this->stride + extraBoundaryPatternBytes;
      s32 numBytesAllocated = 0;

      void * allocatedBuffer = memory.Allocate(numBytesRequested, &numBytesAllocated);

      initialize(numRows,
        numCols,
        reinterpret_cast<s32*>(allocatedBuffer),
        numBytesAllocated,
        useBoundaryFillPatterns);
    }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    void Array_s32::Show(const char * const windowName, const bool waitForKeypress) const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      cv::imshow(windowName, cvMatMirror);
      if(waitForKeypress) {
        cv::waitKey();
      }
    }

    // Returns a templated cv::Mat_ that shares the same buffer with this Array_s32. No data is copied.
    cv::Mat_<s32>& Array_s32::get_CvMat_()
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      return cvMatMirror;
    }
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    // Print out the contents of this Array_s32
    void Array_s32::Print() const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        const s32 * rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          //std::cout << rowPointer[x] << " ";
          printf("%d ", rowPointer[x]); // TODO: make general
        }
        // std::cout << "\n";
        printf("\n");
      }
    }

    // If the Array_s32 was constructed with the useBoundaryFillPatterns=true, then
    // return if any memory was written out of bounds (via fill patterns at the
    // beginning and end).  If the Array_s32 wasn't constructed with the
    // useBoundaryFillPatterns=true, this method always returns true
    bool Array_s32::IsValid() const
    {
      if(this->rawDataPointer == NULL || this->data == NULL) {
        return false;
      }

      if(size[0] < 1 || size[1] < 1) {
        return false;
      }

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1],false);

        for(s32 y=0; y<size[0]; y++) {
          if((reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0]) != FILL_PATTERN_END ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1]) != FILL_PATTERN_END) {
              return false;
          }
        }

        return true;
      } else { // if(useBoundaryFillPatterns) {
        return true; // Technically, we don't know if the Array2d is valid. But we don't know it's NOT valid, so just return true.
      } // if(useBoundaryFillPatterns) { ... else
    }

    // Set every element in the Array_s32 to this value
    // Returns the number of values set
    s32 Array_s32::Set(const s32 value)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        s32 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          rowPointer[x] = value;
        }
      }

      return size[0]*size[1];
    }

    // Parse a space-seperated string, and copy values to this Array_s32.
    // If the string doesn't contain enough elements, the remainder of the Array_s32 will be filled with zeros.
    // Returns the number of values set (not counting extra zeros)
    s32 Array_s32::Set(const char * const values)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      s32 numValuesSet = 0;

      const char * startPointer = values;
      char * endPointer = NULL;

      for(s32 y=0; y<size[0]; y++) {
        s32 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          s32 value = static_cast<s32>(strtol(startPointer, &endPointer, 10));
          if(startPointer != endPointer) {
            rowPointer[x] = value;
            numValuesSet++;
          } else {
            rowPointer[x] = 0;
          }
          startPointer = endPointer;
        }
      }

      return numValuesSet;
    }

    // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
    s32 Array_s32::get_size(s32 dimension) const
    {
      assert(dimension >= 0 && this->rawDataPointer != NULL && this->data != NULL);

      if(dimension > 1 || dimension < 0)
        return 0;

      return size[dimension];
    }

    s32 Array_s32::get_stride() const
    {
      return stride;
    }

    void* Array_s32::get_rawDataPointer()
    {
      return rawDataPointer;
    }

    const void* Array_s32::get_rawDataPointer() const
    {
      return rawDataPointer;
    }

    void Array_s32::initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      this->useBoundaryFillPatterns = useBoundaryFillPatterns;

      if(!rawData) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "input data buffer is NULL");
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->rawDataPointer = rawData;

      const size_t extraBoundaryPatternBytes = useBoundaryFillPatterns ? static_cast<size_t>(HEADER_LENGTH) : 0;
      const s32 extraAlignmentBytes = static_cast<s32>(RoundUp<size_t>(reinterpret_cast<size_t>(rawData)+extraBoundaryPatternBytes, MEMORY_ALIGNMENT) - extraBoundaryPatternBytes - reinterpret_cast<size_t>(rawData));
      const s32 requiredBytes = ComputeRequiredStride(numCols,useBoundaryFillPatterns)*numRows + extraAlignmentBytes;

      if(requiredBytes > dataLength) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "Input data buffer is not large enough. %d bytes is required.", requiredBytes);
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->size[0] = numRows;
      this->size[1] = numCols;

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1], false);
        this->data = reinterpret_cast<s32*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes + HEADER_LENGTH );
        for(s32 y=0; y<size[0]; y++) {
          // Add the fill patterns just before the data on each line
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0] = FILL_PATTERN_START;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1] = FILL_PATTERN_START;

          // And also just after the data (including normal byte-alignment padding)
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0] = FILL_PATTERN_END;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1] = FILL_PATTERN_END;
        }
      } else {
        this->data = reinterpret_cast<s32*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes );
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cvMatMirror = cv::Mat_<s32>(size[0], size[1], data, stride);
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    } // Array_s32::initialize()

    // Set all the buffers and sizes to zero, to signal an invalid array
    void Array_s32::invalidateArray()
    {
      this->size[0] = 0;
      this->size[1] = 0;
      this->stride = 0;
      this->data = NULL;
      this->rawDataPointer = NULL;
    }

    s32 Array_f32::ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0);
      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? (HEADER_LENGTH+FOOTER_LENGTH) : 0);
      return static_cast<s32>(RoundUp<size_t>(sizeof(f32)*numCols, MEMORY_ALIGNMENT)) + extraBoundaryPatternBytes;
    }

    s32 Array_f32::ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0);
      return numRows * Array_f32::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
    }

    Array_f32::Array_f32()
    {
      invalidateArray();
    }

    // Constructor for a Array_f32, pointing to user-allocated data. If the pointer to *data is not
    // aligned to MEMORY_ALIGNMENT, this Array_f32 will start at the next aligned location.
    // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
    // it may make it hard to convert from OpenCV to Array_f32, though the reverse is trivial.
    Array_f32::Array_f32(s32 numRows, s32 numCols, void * data, s32 dataLength, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      initialize(numRows,
        numCols,
        data,
        dataLength,
        useBoundaryFillPatterns);
    }

    Array_f32::Array_f32(s32 numRows, s32 numCols, MemoryStack &memory, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0);

      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? static_cast<s32>(MEMORY_ALIGNMENT) : 0);
      const s32 numBytesRequested = numRows * this->stride + extraBoundaryPatternBytes;
      s32 numBytesAllocated = 0;

      void * allocatedBuffer = memory.Allocate(numBytesRequested, &numBytesAllocated);

      initialize(numRows,
        numCols,
        reinterpret_cast<f32*>(allocatedBuffer),
        numBytesAllocated,
        useBoundaryFillPatterns);
    }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    void Array_f32::Show(const char * const windowName, const bool waitForKeypress) const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      cv::imshow(windowName, cvMatMirror);
      if(waitForKeypress) {
        cv::waitKey();
      }
    }

    // Returns a templated cv::Mat_ that shares the same buffer with this Array_f32. No data is copied.
    cv::Mat_<f32>& Array_f32::get_CvMat_()
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      return cvMatMirror;
    }
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    // Print out the contents of this Array_f32
    void Array_f32::Print() const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        const f32 * rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          //std::cout << rowPointer[x] << " ";
          printf("%f ", rowPointer[x]); // TODO: make general
        }
        // std::cout << "\n";
        printf("\n");
      }
    }

    // If the Array_f32 was constructed with the useBoundaryFillPatterns=true, then
    // return if any memory was written out of bounds (via fill patterns at the
    // beginning and end).  If the Array_f32 wasn't constructed with the
    // useBoundaryFillPatterns=true, this method always returns true
    bool Array_f32::IsValid() const
    {
      if(this->rawDataPointer == NULL || this->data == NULL) {
        return false;
      }

      if(size[0] < 1 || size[1] < 1) {
        return false;
      }

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1],false);

        for(s32 y=0; y<size[0]; y++) {
          if((reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0]) != FILL_PATTERN_END ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1]) != FILL_PATTERN_END) {
              return false;
          }
        }

        return true;
      } else { // if(useBoundaryFillPatterns) {
        return true; // Technically, we don't know if the Array2d is valid. But we don't know it's NOT valid, so just return true.
      } // if(useBoundaryFillPatterns) { ... else
    }

    // Set every element in the Array_f32 to this value
    // Returns the number of values set
    s32 Array_f32::Set(const f32 value)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        f32 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          rowPointer[x] = value;
        }
      }

      return size[0]*size[1];
    }

    // Parse a space-seperated string, and copy values to this Array_f32.
    // If the string doesn't contain enough elements, the remainder of the Array_f32 will be filled with zeros.
    // Returns the number of values set (not counting extra zeros)
    s32 Array_f32::Set(const char * const values)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      s32 numValuesSet = 0;

      const char * startPointer = values;
      char * endPointer = NULL;

      for(s32 y=0; y<size[0]; y++) {
        f32 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          f32 value = static_cast<f32>(strtol(startPointer, &endPointer, 10));
          if(startPointer != endPointer) {
            rowPointer[x] = value;
            numValuesSet++;
          } else {
            rowPointer[x] = 0;
          }
          startPointer = endPointer;
        }
      }

      return numValuesSet;
    }

    // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
    s32 Array_f32::get_size(s32 dimension) const
    {
      assert(dimension >= 0 && this->rawDataPointer != NULL && this->data != NULL);

      if(dimension > 1 || dimension < 0)
        return 0;

      return size[dimension];
    }

    s32 Array_f32::get_stride() const
    {
      return stride;
    }

    void* Array_f32::get_rawDataPointer()
    {
      return rawDataPointer;
    }

    const void* Array_f32::get_rawDataPointer() const
    {
      return rawDataPointer;
    }

    void Array_f32::initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      this->useBoundaryFillPatterns = useBoundaryFillPatterns;

      if(!rawData) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "input data buffer is NULL");
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->rawDataPointer = rawData;

      const size_t extraBoundaryPatternBytes = useBoundaryFillPatterns ? static_cast<size_t>(HEADER_LENGTH) : 0;
      const s32 extraAlignmentBytes = static_cast<s32>(RoundUp<size_t>(reinterpret_cast<size_t>(rawData)+extraBoundaryPatternBytes, MEMORY_ALIGNMENT) - extraBoundaryPatternBytes - reinterpret_cast<size_t>(rawData));
      const s32 requiredBytes = ComputeRequiredStride(numCols,useBoundaryFillPatterns)*numRows + extraAlignmentBytes;

      if(requiredBytes > dataLength) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "Input data buffer is not large enough. %d bytes is required.", requiredBytes);
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->size[0] = numRows;
      this->size[1] = numCols;

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1], false);
        this->data = reinterpret_cast<f32*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes + HEADER_LENGTH );
        for(s32 y=0; y<size[0]; y++) {
          // Add the fill patterns just before the data on each line
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0] = FILL_PATTERN_START;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1] = FILL_PATTERN_START;

          // And also just after the data (including normal byte-alignment padding)
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0] = FILL_PATTERN_END;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1] = FILL_PATTERN_END;
        }
      } else {
        this->data = reinterpret_cast<f32*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes );
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cvMatMirror = cv::Mat_<f32>(size[0], size[1], data, stride);
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    } // Array_f32::initialize()

    // Set all the buffers and sizes to zero, to signal an invalid array
    void Array_f32::invalidateArray()
    {
      this->size[0] = 0;
      this->size[1] = 0;
      this->stride = 0;
      this->data = NULL;
      this->rawDataPointer = NULL;
    }

    s32 Array_f64::ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0);
      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? (HEADER_LENGTH+FOOTER_LENGTH) : 0);
      return static_cast<s32>(RoundUp<size_t>(sizeof(f64)*numCols, MEMORY_ALIGNMENT)) + extraBoundaryPatternBytes;
    }

    s32 Array_f64::ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0);
      return numRows * Array_f64::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
    }

    Array_f64::Array_f64()
    {
      invalidateArray();
    }

    // Constructor for a Array_f64, pointing to user-allocated data. If the pointer to *data is not
    // aligned to MEMORY_ALIGNMENT, this Array_f64 will start at the next aligned location.
    // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
    // it may make it hard to convert from OpenCV to Array_f64, though the reverse is trivial.
    Array_f64::Array_f64(s32 numRows, s32 numCols, void * data, s32 dataLength, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      initialize(numRows,
        numCols,
        data,
        dataLength,
        useBoundaryFillPatterns);
    }

    Array_f64::Array_f64(s32 numRows, s32 numCols, MemoryStack &memory, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0);

      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? static_cast<s32>(MEMORY_ALIGNMENT) : 0);
      const s32 numBytesRequested = numRows * this->stride + extraBoundaryPatternBytes;
      s32 numBytesAllocated = 0;

      void * allocatedBuffer = memory.Allocate(numBytesRequested, &numBytesAllocated);

      initialize(numRows,
        numCols,
        reinterpret_cast<f64*>(allocatedBuffer),
        numBytesAllocated,
        useBoundaryFillPatterns);
    }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    void Array_f64::Show(const char * const windowName, const bool waitForKeypress) const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      cv::imshow(windowName, cvMatMirror);
      if(waitForKeypress) {
        cv::waitKey();
      }
    }

    // Returns a templated cv::Mat_ that shares the same buffer with this Array_f64. No data is copied.
    cv::Mat_<f64>& Array_f64::get_CvMat_()
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      return cvMatMirror;
    }
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    // Print out the contents of this Array_f64
    void Array_f64::Print() const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        const f64 * rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          //std::cout << rowPointer[x] << " ";
          printf("%f ", rowPointer[x]); // TODO: make general
        }
        // std::cout << "\n";
        printf("\n");
      }
    }

    // If the Array_f64 was constructed with the useBoundaryFillPatterns=true, then
    // return if any memory was written out of bounds (via fill patterns at the
    // beginning and end).  If the Array_f64 wasn't constructed with the
    // useBoundaryFillPatterns=true, this method always returns true
    bool Array_f64::IsValid() const
    {
      if(this->rawDataPointer == NULL || this->data == NULL) {
        return false;
      }

      if(size[0] < 1 || size[1] < 1) {
        return false;
      }

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1],false);

        for(s32 y=0; y<size[0]; y++) {
          if((reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0]) != FILL_PATTERN_END ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1]) != FILL_PATTERN_END) {
              return false;
          }
        }

        return true;
      } else { // if(useBoundaryFillPatterns) {
        return true; // Technically, we don't know if the Array2d is valid. But we don't know it's NOT valid, so just return true.
      } // if(useBoundaryFillPatterns) { ... else
    }

    // Set every element in the Array_f64 to this value
    // Returns the number of values set
    s32 Array_f64::Set(const f64 value)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        f64 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          rowPointer[x] = value;
        }
      }

      return size[0]*size[1];
    }

    // Parse a space-seperated string, and copy values to this Array_f64.
    // If the string doesn't contain enough elements, the remainder of the Array_f64 will be filled with zeros.
    // Returns the number of values set (not counting extra zeros)
    s32 Array_f64::Set(const char * const values)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      s32 numValuesSet = 0;

      const char * startPointer = values;
      char * endPointer = NULL;

      for(s32 y=0; y<size[0]; y++) {
        f64 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          f64 value = static_cast<f64>(strtol(startPointer, &endPointer, 10));
          if(startPointer != endPointer) {
            rowPointer[x] = value;
            numValuesSet++;
          } else {
            rowPointer[x] = 0;
          }
          startPointer = endPointer;
        }
      }

      return numValuesSet;
    }

    // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
    s32 Array_f64::get_size(s32 dimension) const
    {
      assert(dimension >= 0 && this->rawDataPointer != NULL && this->data != NULL);

      if(dimension > 1 || dimension < 0)
        return 0;

      return size[dimension];
    }

    s32 Array_f64::get_stride() const
    {
      return stride;
    }

    void* Array_f64::get_rawDataPointer()
    {
      return rawDataPointer;
    }

    const void* Array_f64::get_rawDataPointer() const
    {
      return rawDataPointer;
    }

    void Array_f64::initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      this->useBoundaryFillPatterns = useBoundaryFillPatterns;

      if(!rawData) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "input data buffer is NULL");
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->rawDataPointer = rawData;

      const size_t extraBoundaryPatternBytes = useBoundaryFillPatterns ? static_cast<size_t>(HEADER_LENGTH) : 0;
      const s32 extraAlignmentBytes = static_cast<s32>(RoundUp<size_t>(reinterpret_cast<size_t>(rawData)+extraBoundaryPatternBytes, MEMORY_ALIGNMENT) - extraBoundaryPatternBytes - reinterpret_cast<size_t>(rawData));
      const s32 requiredBytes = ComputeRequiredStride(numCols,useBoundaryFillPatterns)*numRows + extraAlignmentBytes;

      if(requiredBytes > dataLength) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "Input data buffer is not large enough. %d bytes is required.", requiredBytes);
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->size[0] = numRows;
      this->size[1] = numCols;

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1], false);
        this->data = reinterpret_cast<f64*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes + HEADER_LENGTH );
        for(s32 y=0; y<size[0]; y++) {
          // Add the fill patterns just before the data on each line
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0] = FILL_PATTERN_START;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1] = FILL_PATTERN_START;

          // And also just after the data (including normal byte-alignment padding)
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0] = FILL_PATTERN_END;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1] = FILL_PATTERN_END;
        }
      } else {
        this->data = reinterpret_cast<f64*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes );
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cvMatMirror = cv::Mat_<f64>(size[0], size[1], data, stride);
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    } // Array_f64::initialize()

    // Set all the buffers and sizes to zero, to signal an invalid array
    void Array_f64::invalidateArray()
    {
      this->size[0] = 0;
      this->size[1] = 0;
      this->stride = 0;
      this->data = NULL;
      this->rawDataPointer = NULL;
    }

    s32 Array_Point_s16::ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0);
      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? (HEADER_LENGTH+FOOTER_LENGTH) : 0);
      return static_cast<s32>(RoundUp<size_t>(sizeof(Point_s16)*numCols, MEMORY_ALIGNMENT)) + extraBoundaryPatternBytes;
    }

    s32 Array_Point_s16::ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0);
      return numRows * Array_Point_s16::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
    }

    Array_Point_s16::Array_Point_s16()
    {
      invalidateArray();
    }

    // Constructor for a Array_Point_s16, pointing to user-allocated data. If the pointer to *data is not
    // aligned to MEMORY_ALIGNMENT, this Array_Point_s16 will start at the next aligned location.
    // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
    // it may make it hard to convert from OpenCV to Array_Point_s16, though the reverse is trivial.
    Array_Point_s16::Array_Point_s16(s32 numRows, s32 numCols, void * data, s32 dataLength, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      initialize(numRows,
        numCols,
        data,
        dataLength,
        useBoundaryFillPatterns);
    }

    Array_Point_s16::Array_Point_s16(s32 numRows, s32 numCols, MemoryStack &memory, bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0);

      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? static_cast<s32>(MEMORY_ALIGNMENT) : 0);
      const s32 numBytesRequested = numRows * this->stride + extraBoundaryPatternBytes;
      s32 numBytesAllocated = 0;

      void * allocatedBuffer = memory.Allocate(numBytesRequested, &numBytesAllocated);

      initialize(numRows,
        numCols,
        reinterpret_cast<Point_s16*>(allocatedBuffer),
        numBytesAllocated,
        useBoundaryFillPatterns);
    }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    void Array_Point_s16::Show(const char * const windowName, const bool waitForKeypress) const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      cv::imshow(windowName, cvMatMirror);
      if(waitForKeypress) {
        cv::waitKey();
      }
    }

    // Returns a templated cv::Mat_ that shares the same buffer with this Array_Point_s16. No data is copied.
    cv::Mat_<Point_s16>& Array_Point_s16::get_CvMat_()
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      return cvMatMirror;
    }
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    // Print out the contents of this Array_Point_s16
    void Array_Point_s16::Print() const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        const Point_s16 * rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          //std::cout << rowPointer[x] << " ";
          printf("(%d,%d) ", rowPointer[x].x, rowPointer[x].y); // TODO: make general
        }
        // std::cout << "\n";
        printf("\n");
      }
    }

    // If the Array_Point_s16 was constructed with the useBoundaryFillPatterns=true, then
    // return if any memory was written out of bounds (via fill patterns at the
    // beginning and end).  If the Array_Point_s16 wasn't constructed with the
    // useBoundaryFillPatterns=true, this method always returns true
    bool Array_Point_s16::IsValid() const
    {
      if(this->rawDataPointer == NULL || this->data == NULL) {
        return false;
      }

      if(size[0] < 1 || size[1] < 1) {
        return false;
      }

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1],false);

        for(s32 y=0; y<size[0]; y++) {
          if((reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0]) != FILL_PATTERN_END ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1]) != FILL_PATTERN_END) {
              return false;
          }
        }

        return true;
      } else { // if(useBoundaryFillPatterns) {
        return true; // Technically, we don't know if the Array2d is valid. But we don't know it's NOT valid, so just return true.
      } // if(useBoundaryFillPatterns) { ... else
    }

    // Set every element in the Array_Point_s16 to this value
    // Returns the number of values set
    s32 Array_Point_s16::Set(const Point_s16 value)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        Point_s16 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          rowPointer[x] = value;
        }
      }

      return size[0]*size[1];
    }

    // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
    s32 Array_Point_s16::get_size(s32 dimension) const
    {
      assert(dimension >= 0 && this->rawDataPointer != NULL && this->data != NULL);

      if(dimension > 1 || dimension < 0)
        return 0;

      return size[dimension];
    }

    s32 Array_Point_s16::get_stride() const
    {
      return stride;
    }

    void* Array_Point_s16::get_rawDataPointer()
    {
      return rawDataPointer;
    }

    const void* Array_Point_s16::get_rawDataPointer() const
    {
      return rawDataPointer;
    }

    void Array_Point_s16::initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      this->useBoundaryFillPatterns = useBoundaryFillPatterns;

      if(!rawData) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "input data buffer is NULL");
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->rawDataPointer = rawData;

      const size_t extraBoundaryPatternBytes = useBoundaryFillPatterns ? static_cast<size_t>(HEADER_LENGTH) : 0;
      const s32 extraAlignmentBytes = static_cast<s32>(RoundUp<size_t>(reinterpret_cast<size_t>(rawData)+extraBoundaryPatternBytes, MEMORY_ALIGNMENT) - extraBoundaryPatternBytes - reinterpret_cast<size_t>(rawData));
      const s32 requiredBytes = ComputeRequiredStride(numCols,useBoundaryFillPatterns)*numRows + extraAlignmentBytes;

      if(requiredBytes > dataLength) {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        DASError("Anki.Array2d.initialize", "Input data buffer is not large enough. %d bytes is required.", requiredBytes);
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
        invalidateArray();
        return;
      }

      this->size[0] = numRows;
      this->size[1] = numCols;

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1], false);
        this->data = reinterpret_cast<Point_s16*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes + HEADER_LENGTH );
        for(s32 y=0; y<size[0]; y++) {
          // Add the fill patterns just before the data on each line
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0] = FILL_PATTERN_START;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1] = FILL_PATTERN_START;

          // And also just after the data (including normal byte-alignment padding)
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0] = FILL_PATTERN_END;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1] = FILL_PATTERN_END;
        }
      } else {
        this->data = reinterpret_cast<Point_s16*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes );
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cvMatMirror = cv::Mat_<Point_s16>(size[0], size[1], data, stride);
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    } // Array_Point_s16::initialize()

    // Set all the buffers and sizes to zero, to signal an invalid array
    void Array_Point_s16::invalidateArray()
    {
      this->size[0] = 0;
      this->size[1] = 0;
      this->stride = 0;
      this->data = NULL;
      this->rawDataPointer = NULL;
    }

#ifndef USE_ARRAY_INLINE_POINTERS

    // Pointer to the data, at a given (y,x) location
    const u8* Array_u8::Pointer(const s32 index0, const s32 index1) const
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<const u8*>( reinterpret_cast<const char*>(this->data) +
        index1*sizeof(u8) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    u8* Array_u8::Pointer(const s32 index0, const s32 index1)
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<u8*>( reinterpret_cast<char*>(this->data) +
        index1*sizeof(u8) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    const u8* Array_u8::Pointer(const Point_s16 &point) const
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    // Pointer to the data, at a given (y,x) location
    u8* Array_u8::Pointer(const Point_s16 &point)
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    // Pointer to the data, at a given (y,x) location
    const s8* Array_s8::Pointer(const s32 index0, const s32 index1) const
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<const s8*>( reinterpret_cast<const char*>(this->data) +
        index1*sizeof(s8) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    s8* Array_s8::Pointer(const s32 index0, const s32 index1)
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<s8*>( reinterpret_cast<char*>(this->data) +
        index1*sizeof(s8) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    const s8* Array_s8::Pointer(const Point_s16 &point) const
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    // Pointer to the data, at a given (y,x) location
    s8* Array_s8::Pointer(const Point_s16 &point)
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    // Pointer to the data, at a given (y,x) location
    const u16* Array_u16::Pointer(const s32 index0, const s32 index1) const
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<const u16*>( reinterpret_cast<const char*>(this->data) +
        index1*sizeof(u16) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    u16* Array_u16::Pointer(const s32 index0, const s32 index1)
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<u16*>( reinterpret_cast<char*>(this->data) +
        index1*sizeof(u16) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    const u16* Array_u16::Pointer(const Point_s16 &point) const
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    // Pointer to the data, at a given (y,x) location
    u16* Array_u16::Pointer(const Point_s16 &point)
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    // Pointer to the data, at a given (y,x) location
    const s16* Array_s16::Pointer(const s32 index0, const s32 index1) const
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<const s16*>( reinterpret_cast<const char*>(this->data) +
        index1*sizeof(s16) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    s16* Array_s16::Pointer(const s32 index0, const s32 index1)
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<s16*>( reinterpret_cast<char*>(this->data) +
        index1*sizeof(s16) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    const s16* Array_s16::Pointer(const Point_s16 &point) const
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    // Pointer to the data, at a given (y,x) location
    s16* Array_s16::Pointer(const Point_s16 &point)
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    // Pointer to the data, at a given (y,x) location
    const u32* Array_u32::Pointer(const s32 index0, const s32 index1) const
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<const u32*>( reinterpret_cast<const char*>(this->data) +
        index1*sizeof(u32) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    u32* Array_u32::Pointer(const s32 index0, const s32 index1)
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) +
        index1*sizeof(u32) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    const u32* Array_u32::Pointer(const Point_s16 &point) const
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    // Pointer to the data, at a given (y,x) location
    u32* Array_u32::Pointer(const Point_s16 &point)
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }
    // Pointer to the data, at a given (y,x) location
    const s32* Array_s32::Pointer(const s32 index0, const s32 index1) const
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<const s32*>( reinterpret_cast<const char*>(this->data) +
        index1*sizeof(s32) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    s32* Array_s32::Pointer(const s32 index0, const s32 index1)
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<s32*>( reinterpret_cast<char*>(this->data) +
        index1*sizeof(s32) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    const s32* Array_s32::Pointer(const Point_s16 &point) const
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    // Pointer to the data, at a given (y,x) location
    s32* Array_s32::Pointer(const Point_s16 &point)
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    // Pointer to the data, at a given (y,x) location
    const f32* Array_f32::Pointer(const s32 index0, const s32 index1) const
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<const f32*>( reinterpret_cast<const char*>(this->data) +
        index1*sizeof(f32) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    f32* Array_f32::Pointer(const s32 index0, const s32 index1)
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<f32*>( reinterpret_cast<char*>(this->data) +
        index1*sizeof(f32) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    const f32* Array_f32::Pointer(const Point_s16 &point) const
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    // Pointer to the data, at a given (y,x) location
    f32* Array_f32::Pointer(const Point_s16 &point)
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    // Pointer to the data, at a given (y,x) location
    const f64* Array_f64::Pointer(const s32 index0, const s32 index1) const
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<const f64*>( reinterpret_cast<const char*>(this->data) +
        index1*sizeof(f64) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    f64* Array_f64::Pointer(const s32 index0, const s32 index1)
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<f64*>( reinterpret_cast<char*>(this->data) +
        index1*sizeof(f64) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    const f64* Array_f64::Pointer(const Point_s16 &point) const
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    // Pointer to the data, at a given (y,x) location
    f64* Array_f64::Pointer(const Point_s16 &point)
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    // Pointer to the data, at a given (y,x) location
    const Point_s16* Array_Point_s16::Pointer(const s32 index0, const s32 index1) const
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<const Point_s16*>( reinterpret_cast<const char*>(this->data) +
        index1*sizeof(Point_s16) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    Point_s16* Array_Point_s16::Pointer(const s32 index0, const s32 index1)
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<Point_s16*>( reinterpret_cast<char*>(this->data) +
        index1*sizeof(Point_s16) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    const Point_s16* Array_Point_s16::Pointer(const Point_s16 &point) const
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    // Pointer to the data, at a given (y,x) location
    Point_s16* Array_Point_s16::Pointer(const Point_s16 &point)
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }
#endif // USE_ARRAY_INLINE_POINTERS
  } // namespace Embedded
} // namespace Anki