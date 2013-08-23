#ifndef _ANKICORETECH_COMMON_MATRIX_H_
#define _ANKICORETECH_COMMON_MATRIX_H_

#include "anki/common/config.h"
#include "anki/common/utilities.h"
#include "anki/common/memory.h"
#include "anki/common/DASlight.h"
#include "anki/common/dataStructures.h"

#include <iostream>
#include <assert.h>

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

namespace Anki
{
  template<typename T> class Point2;
  
#pragma mark --- Array2dUnmanaged Class Definition ---

  // A Array2dUnmanaged is a lightweight templated class for holding two dimensional data. It does no
  // reference counting, or allocating/freeing from the heap. The data from Array2dUnmanaged is
  // OpenCV-compatible, and accessable via get_CvMat_(). The matlabInterface.h can send and receive
  // Array2dUnmanaged objects from Matlab
  template<typename T>
  class Array2dUnmanaged
  {
  public:
    static s32 ComputeRequiredStride(s32 numCols, bool useBoundaryFillPatterns);

    static s32 ComputeMinimumRequiredMemory(s32 numRows, s32 numCols, bool useBoundaryFillPatterns);

    Array2dUnmanaged();

    // Constructor for a Array2dUnmanaged, pointing to user-allocated data. If the pointer to *data is not
    // aligned to Anki::MEMORY_ALIGNMENT, this Array2dUnmanaged will start at the next aligned location.
    // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
    // it may make it hard to convert from OpenCV to Anki::Array2dUnmanaged, though the reverse is trivial.
    Array2dUnmanaged(s32 numRows, s32 numCols, void * data, s32 dataLength, bool useBoundaryFillPatterns=false);

    // Constructor for a Array2dUnmanaged, pointing to user-allocated MemoryStack
    Array2dUnmanaged(s32 numRows, s32 numCols, MemoryStack &memory, bool useBoundaryFillPatterns=false);

    // Pointer to the data, at a given (y,x) location
    const inline T* Pointer(s32 index0, s32 index1) const;

    // Pointer to the data, at a given (y,x) location
    inline T* Pointer(s32 index0, s32 index1);

    // Pointer to the data, at a given (y,x) location
    template<typename TPoint> const inline T* Pointer(Point2<TPoint> point) const;

    // Pointer to the data, at a given (y,x) location
    template<typename TPoint> inline T* Pointer(Point2<TPoint> point);

#if defined(ANKICORETECH_USE_OPENCV)
    void Show(const std::string windowName, bool waitForKeypress) const;

    // Returns a templated cv::Mat_ that shares the same buffer with this Anki::Array2dUnmanaged. No data is copied.
    cv::Mat_<T>& get_CvMat_();
#endif // #if defined(ANKICORETECH_USE_OPENCV)

    // Print out the contents of this Array2dUnmanaged
    void Print() const;

    // If the Array2dUnmanaged was constructed with the useBoundaryFillPatterns=true, then
    // return if any memory was written out of bounds (via fill patterns at the
    // beginning and end).  If the Array2dUnmanaged wasn't constructed with the
    // useBoundaryFillPatterns=true, this method always returns true
    bool IsValid() const;

    // Set every element in the Array2dUnmanaged to this value
    // Returns the number of values set
    s32 Set(T value);

    // Parse a space-seperated string, and copy values to this Array2dUnmanaged.
    // If the string doesn't contain enough elements, the remainded of the Array2dUnmanaged will be filled with zeros.
    // Returns the number of values set (not counting extra zeros)
    s32 Set(const std::string values);

    // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
    s32 get_size(s32 dimension) const;
    
    inline s32 get_nrows(void) const;
    inline s32 get_ncols(void) const;
    inline s32 get_numElements(void) const;
    
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

    T * data;

    // To enforce alignment, rawDataPointer may be slightly before T * data.
    // If the inputted data buffer was from malloc, this is the pointer that
    // should be used to free.
    void * rawDataPointer;

#if defined(ANKICORETECH_USE_OPENCV)
    cv::Mat_<T> cvMatMirror;
#endif // #if defined(ANKICORETECH_USE_OPENCV)

    void initialize(s32 numRows, s32 numCols, void * rawData, s32 dataLength,
      bool useBoundaryFillPatterns);
  }; // class Array2dUnmanaged

# pragma mark --- Array2dUnmanagedFixedPoint Definition ---

  template<typename T>
  class Array2dUnmanagedFixedPoint : public Array2dUnmanaged<T>
  {
  public:
    Array2dUnmanagedFixedPoint();

    // Constructor for a Array2dUnmanagedFixedPoint, pointing to user-allocated data.
    Array2dUnmanagedFixedPoint(s32 numRows, s32 numCols, s32 numFractionalBits,
      void * data, s32 dataLength, bool useBoundaryFillPatterns=false);

    // Constructor for a Array2dUnmanagedFixedPoint, pointing to user-allocated MemoryStack
    Array2dUnmanagedFixedPoint(s32 numRows, s32 numCols, s32 numFractionalBits,
      MemoryStack &memory, bool useBoundaryFillPatterns=false);

    Array2dUnmanagedFixedPoint(Array2dUnmanaged<T> &mat);

    bool IsValid() const;

    s32 get_numFractionalBits() const;

  protected:
    s32 numFractionalBits;
  }; // class Array2dUnmanagedFixedPoint

  
#pragma mark --- Array2d(Managed) Class Definition ---
  
  // A version of Array2d that offers internal memory allocation/deallocation,
  // but retains the embedded-platform-friendly memory alignment of the parent
  // class:
  template<typename T>
  class Array2d : public Array2dUnmanaged<T>
  {
  public:
    Array2d();
    Array2d(s32 nrows, s32 ncols, bool useBoundaryFillPatterns = false);
    
  protected:
    
    using Array2dUnmanaged<T>::initialize;
    
    // Remove non-const access to raw data pointer inherited from parent class
    void* get_rawDataPointer();
    
    // Currently using OpenCv to manage the data. It will allocate the data
    // according to the type, and most importantly for us, it gives us nice
    // smart pointers that handle the deallocation when nobody is still using
    // the data, allowing us to get "free" assignment without lots of unneeded
    // copying.
    cv::Mat_<T> dataManager;
    int refCount;
    
  }; // class Array2d(Managed)
  
  
  // An alternate name to explicitly differentiate Managed vs. Unmanaged
  // Unfortunately can't use typedef simply (?):
  //    typedef Array2d Array2dManaged;
  // But this #define seems like a *horrible* idea:
  // TODO: Better solution to get an "alias" for Array2dManaged
#define Array2dManaged Array2d
  
  
#pragma mark --- FixedLengthList Definition ---

  template<typename T>
  class FixedLengthList : public Array2dUnmanaged<T>
  {
  public:
    FixedLengthList();

    // Constructor for a FixedLengthList, pointing to user-allocated data.
    FixedLengthList(s32 maximumSize, void * data, s32 dataLength,
      bool useBoundaryFillPatterns=false);

    // Constructor for a FixedLengthList, pointing to user-allocated MemoryStack
    FixedLengthList(s32 maximumSize, MemoryStack &memory,
      bool useBoundaryFillPatterns=false);

    bool IsValid() const;

    Result PushBack(const T &value);

    // Will act as a normal pop, except when the list is empty. Then subsequent
    // calls will keep returning the first value in the list.
    T PopBack();

    void Clear();

    // Pointer to the data, at a given location
    inline T* Pointer(s32 index);

    // Pointer to the data, at a given location
    inline const T* Pointer(s32 index) const;

    s32 get_maximumSize() const;

    s32 get_size() const;

  protected:
    s32 capacityUsed;
  }; // class FixedLengthList

#pragma mark --- Array2dUnmanaged Implementations ---

  template<typename T>
  s32 Array2dUnmanaged<T>::ComputeRequiredStride(s32 numCols, bool useBoundaryFillPatterns)
  {
    assert(numCols > 0);
    const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? (HEADER_LENGTH+FOOTER_LENGTH) : 0);
    return static_cast<s32>(Anki::RoundUp<size_t>(sizeof(T)*numCols, Anki::MEMORY_ALIGNMENT)) + extraBoundaryPatternBytes;
  }

  template<typename T>
  s32 Array2dUnmanaged<T>::ComputeMinimumRequiredMemory(s32 numRows, s32 numCols,
    bool useBoundaryFillPatterns)
  {
    assert(numCols > 0 && numRows > 0);
    return numRows * Anki::Array2dUnmanaged<T>::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
  }

  template<typename T>
  Array2dUnmanaged<T>::Array2dUnmanaged()
    : stride(0), useBoundaryFillPatterns(false), data(NULL), rawDataPointer(NULL)
  {
    this->size[0] = 0;
    this->size[1] = 0;
  }

  template<typename T>
  Array2dUnmanaged<T>::Array2dUnmanaged(s32 numRows, s32 numCols, void * data, s32 dataLength,
    bool useBoundaryFillPatterns)
    : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
  {
    assert(numCols > 0 && numRows > 0 && dataLength > 0);

    initialize(numRows,
      numCols,
      data,
      dataLength,
      useBoundaryFillPatterns);
  }

  template<typename T>
  Array2dUnmanaged<T>::Array2dUnmanaged(s32 numRows, s32 numCols, MemoryStack &memory,
    bool useBoundaryFillPatterns)
    : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
  {
    assert(numCols > 0 && numRows > 0);

    const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? static_cast<s32>(Anki::MEMORY_ALIGNMENT) : 0);
    const s32 numBytesRequested = numRows * this->stride + extraBoundaryPatternBytes;
    s32 numBytesAllocated = 0;

    void * allocatedBuffer = memory.Allocate(numBytesRequested, &numBytesAllocated);

    initialize(numRows,
      numCols,
      reinterpret_cast<T*>(allocatedBuffer),
      numBytesAllocated,
      useBoundaryFillPatterns);
  }

  template<typename T>
  const T* Array2dUnmanaged<T>::Pointer(s32 index0, s32 index1) const
  {
    assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
      this->rawDataPointer != NULL && this->data != NULL);

    return reinterpret_cast<const T*>( reinterpret_cast<const char*>(this->data) +
      index1*sizeof(T) + index0*stride );
  }

  template<typename T>
  T* Array2dUnmanaged<T>::Pointer(s32 index0, s32 index1)
  {
    assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
      this->rawDataPointer != NULL && this->data != NULL);

    return reinterpret_cast<T*>( reinterpret_cast<char*>(this->data) +
      index1*sizeof(T) + index0*stride );
  }

  template<typename T>
  template<typename TPoint>
  const T* Array2dUnmanaged<T>::Pointer(Point2<TPoint> point) const
  {
    return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
  }

  template<typename T>
  template<typename TPoint>
  T* Array2dUnmanaged<T>::Pointer(Point2<TPoint> point)
  {
    return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
  }

#if defined(ANKICORETECH_USE_OPENCV)
  template<typename T>
  void Array2dUnmanaged<T>::Show(const std::string windowName, bool waitForKeypress) const {
    assert(this->rawDataPointer != NULL && this->data != NULL);
    cv::imshow(windowName, cvMatMirror);
    if(waitForKeypress) {
      cv::waitKey();
    }
  }

  template<typename T>
  cv::Mat_<T>& Array2dUnmanaged<T>::get_CvMat_()
  {
    assert(this->rawDataPointer != NULL && this->data != NULL);
    return cvMatMirror;
  }
#endif // #if defined(ANKICORETECH_USE_OPENCV)

  template<typename T>
  void Array2dUnmanaged<T>::Print() const
  {
    assert(this->rawDataPointer != NULL && this->data != NULL);

    for(s32 y=0; y<size[0]; y++) {
      const T * rowPointer = Pointer(y, 0);
      for(s32 x=0; x<size[1]; x++) {
        std::cout << rowPointer[x] << " ";
      }
      std::cout << "\n";
    }
  }

  template<typename T>
  bool Array2dUnmanaged<T>::IsValid() const
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
      return true; // Technically, we don't know if the Array2dUnmanaged is valid. But we don't know it's NOT valid, so just return true.
    } // if(useBoundaryFillPatterns) { ... else
  }
  template<typename T>
  s32 Array2dUnmanaged<T>::Set(T value)
  {
    assert(this->rawDataPointer != NULL && this->data != NULL);

    for(s32 y=0; y<size[0]; y++) {
      T * restrict rowPointer = Pointer(y, 0);
      for(s32 x=0; x<size[1]; x++) {
        rowPointer[x] = value;
      }
    }

    return size[0]*size[1];
  }

  template<typename T>
  s32 Array2dUnmanaged<T>::Set(const std::string values)
  {
    assert(this->rawDataPointer != NULL && this->data != NULL);

    std::istringstream iss(values);
    s32 numValuesSet = 0;

    for(s32 y=0; y<size[0]; y++) {
      T * restrict rowPointer = Pointer(y, 0);
      for(s32 x=0; x<size[1]; x++) {
        T value;
        if(iss >> value) {
          rowPointer[x] = value;
          numValuesSet++;
        } else {
          rowPointer[x] = 0;
        }
      }
    }

    return numValuesSet;
  }

  template<typename T>
  s32 Array2dUnmanaged<T>::get_size(s32 dimension) const
  {
    assert(dimension >= 0 && this->rawDataPointer != NULL && this->data != NULL);

    if(dimension > 1 || dimension < 0)
      return 0;

    return size[dimension];
  }

  template<typename T>
  s32 Array2dUnmanaged<T>::get_nrows(void) const
  {
    return this->size[0];
  }
  
  template<typename T>
  s32 Array2dUnmanaged<T>::get_ncols(void) const
  {
    return this->size[1];
  }
  
  template<typename T>
  s32 Array2dUnmanaged<T>::get_numElements(void) const
  {
    return (this->size[0] * this->size[1]);
  }
  
  template<typename T>
  s32 Array2dUnmanaged<T>::get_stride() const
  {
    return stride;
  }

  template<typename T>
  void* Array2dUnmanaged<T>::get_rawDataPointer()
  {
    return rawDataPointer;
  }

  template<typename T>
  const void* Array2dUnmanaged<T>::get_rawDataPointer() const
  {
    return rawDataPointer;
  }

  template<typename T>
  void Array2dUnmanaged<T>::initialize(s32 numRows, s32 numCols, void * rawData,
    s32 dataLength, bool useBoundaryFillPatterns)
  {
    assert(numCols > 0 && numRows > 0 && dataLength > 0);

    this->useBoundaryFillPatterns = useBoundaryFillPatterns;

    if(!rawData) {
      DASError("Anki.Array2dUnmanaged.initialize", "input data buffer is NULL");
      this->size[0] = 0;
      this->size[1] = 0;
      this->data = NULL;
      this->rawDataPointer = NULL;
      return;
    }

    this->rawDataPointer = rawData;

    const size_t extraBoundaryPatternBytes = useBoundaryFillPatterns ? static_cast<size_t>(HEADER_LENGTH) : 0;
    const s32 extraAlignmentBytes = static_cast<s32>(RoundUp<size_t>(reinterpret_cast<size_t>(rawData)+extraBoundaryPatternBytes, Anki::MEMORY_ALIGNMENT) - extraBoundaryPatternBytes - reinterpret_cast<size_t>(rawData));
    const s32 requiredBytes = ComputeRequiredStride(numCols,useBoundaryFillPatterns)*numRows + extraAlignmentBytes;

    if(requiredBytes > dataLength) {
      DASError("Anki.Array2dUnmanaged.initialize", "Input data buffer is not large enough. %d bytes is required.", requiredBytes);
      this->size[0] = 0;
      this->size[1] = 0;
      this->data = NULL;
      this->rawDataPointer = NULL;
      return;
    }

    this->size[0] = numRows;
    this->size[1] = numCols;

    if(useBoundaryFillPatterns) {
      const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1], false);
      this->data = reinterpret_cast<T*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes + HEADER_LENGTH );
      for(s32 y=0; y<size[0]; y++) {
        // Add the fill patterns just before the data on each line
        reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0] = FILL_PATTERN_START;
        reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1] = FILL_PATTERN_START;

        // And also just after the data (including normal byte-alignment padding)
        reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0] = FILL_PATTERN_END;
        reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1] = FILL_PATTERN_END;
      }
    } else {
      this->data = reinterpret_cast<T*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes );
    }

#if defined(ANKICORETECH_USE_OPENCV)
    cvMatMirror = cv::Mat_<T>(size[0], size[1], data, stride);
#endif // #if defined(ANKICORETECH_USE_OPENCV)
  } // Array2dUnmanaged<T>::initialize()

#pragma mark --- Matrix Specializations ---

  template<> void Array2dUnmanaged<u8>::Print() const;
  template<> s32 Array2dUnmanaged<u8>::Set(const std::string values);

#pragma mark --- Array2dUnmanagedFixedPoint Implementations ---

  template<typename T>
  Array2dUnmanagedFixedPoint<T>::Array2dUnmanagedFixedPoint()
    : Array2dUnmanaged<T>(), numFractionalBits(0)
  {
  }

  
#pragma mark --- Array2d(Managed) Implementation ---
  
  // This constructor uses the OpenCv managedData matrix to allocate the data,
  // and then points the inherited (unmanaged) data pointer to that.
  template<typename T>
  Array2d<T>::Array2d(s32 numRows, s32 numCols, bool useBoundaryFillPatterns)
  {
    
    // Allocate a memory-aligned array the way we'd expect a user of the
    // UNmanaged parent class to do it:
    this->stride = Array2dUnmanaged<T>::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
    s32 dataLength = Array2dUnmanaged<T>::ComputeMinimumRequiredMemory(numRows, numCols, useBoundaryFillPatterns);
    void *rawData = cv::fastMalloc(dataLength);
    
    initialize(numRows, numCols, rawData, dataLength, useBoundaryFillPatterns);
    
    // Create an OpenCv header around our data array
    this->dataManager = cv::Mat_<T>(numRows, numCols, (T *) this->data);
    
    // "Trick" OpenCv into doing reference counting, even though we passed in
    // our own data array.  So now when this class destructs and dataManager
    // destructs, it will free this->data for us.
    this->refCount = 1;
    this->dataManager.refcount = &(this->refCount);
    
  } // Constructor: Array2d(rows,cols)
  


#pragma mark --- FixedPointMatrix Implementations ---
  
  template<typename T>
  Array2dUnmanagedFixedPoint<T>::Array2dUnmanagedFixedPoint(s32 numRows, s32 numCols, s32 numFractionalBits,
    void * data, s32 dataLength, bool useBoundaryFillPatterns)
    : Array2dUnmanaged<T>(numRows, numCols, data, dataLength, useBoundaryFillPatterns), numFractionalBits(numFractionalBits)
  {
    assert(numFractionalBits >= 0 && numFractionalBits <= (sizeof(T)*8));
  }

  template<typename T>
  Array2dUnmanagedFixedPoint<T>::Array2dUnmanagedFixedPoint(s32 numRows, s32 numCols, s32 numFractionalBits,
    MemoryStack &memory, bool useBoundaryFillPatterns)
    : Array2dUnmanaged<T>(numRows, numCols, memory, useBoundaryFillPatterns), numFractionalBits(numFractionalBits)
  {
    assert(numFractionalBits >= 0 && numFractionalBits <= (sizeof(T)*8));
  }

  template<typename T>
  Array2dUnmanagedFixedPoint<T>::Array2dUnmanagedFixedPoint(Array2dUnmanaged<T> &mat)
    : Array2dUnmanaged<T>(mat), numFractionalBits(0)
  {
  }

  template<typename T>
  bool Array2dUnmanagedFixedPoint<T>::IsValid() const
  {
    if(numFractionalBits > (sizeof(T)*8)) {
      return false;
    }

    return Array2dUnmanaged<T>::IsValid();
  }

  template<typename T>
  s32 Array2dUnmanagedFixedPoint<T>::get_numFractionalBits() const
  {
    return numFractionalBits;
  }

#pragma mark --- FixedLengthList Implementations

  template<typename T>
  FixedLengthList<T>::FixedLengthList()
    : Array2dUnmanaged<T>(), capacityUsed(0)
  {
  }

  template<typename T>
  FixedLengthList<T>::FixedLengthList(s32 maximumSize, void * data, s32 dataLength,
    bool useBoundaryFillPatterns)
    : Array2dUnmanaged<T>(1, maximumSize, data, dataLength, useBoundaryFillPatterns), capacityUsed(0)
  {
  }

  template<typename T>
  FixedLengthList<T>::FixedLengthList(s32 maximumSize, MemoryStack &memory,
    bool useBoundaryFillPatterns)
    : Array2dUnmanaged<T>(1, maximumSize, memory, useBoundaryFillPatterns), capacityUsed(0)
  {
  }

  template<typename T>
  bool FixedLengthList<T>::IsValid() const
  {
    if(capacityUsed > this->get_maximumSize()) {
      return false;
    }

    return Array2dUnmanaged<T>::IsValid();
  }

  template<typename T>
  Result FixedLengthList<T>::PushBack(const T &value)
  {
    if(capacityUsed >= this->get_maximumSize()) {
      return RESULT_FAIL;
    }

    *this->Pointer(capacityUsed) = value;
    capacityUsed++;

    return RESULT_OK;
  }

  template<typename T>
  T FixedLengthList<T>::PopBack()
  {
    if(capacityUsed == 0) {
      return *this->Pointer(0);
    }

    const T value = *this->Pointer(capacityUsed-1);
    capacityUsed--;

    return value;
  }

  template<typename T>
  void FixedLengthList<T>::Clear()
  {
    this->capacityUsed = 0;
  }

  template<typename T>
  T* FixedLengthList<T>::Pointer(s32 index)
  {
    return Array2dUnmanaged<T>::Pointer(0, index);
  }

  // Pointer to the data, at a given location
  template<typename T>
  const T* FixedLengthList<T>::Pointer(s32 index) const
  {
    return Array2dUnmanaged<T>::Pointer(0, index);
  }

  template<typename T>
  s32 FixedLengthList<T>::get_maximumSize() const
  {
    return Array2dUnmanaged<T>::get_size(1);
  }

  template<typename T>
  s32 FixedLengthList<T>::get_size() const
  {
    return capacityUsed;
  }

#pragma mark --- Helper Functions ---

  template<typename T1, typename T2>
  bool AreMatricesEqual_Size(const Array2dUnmanaged<T1> &mat1, const Array2dUnmanaged<T2> &mat2)
  {
    if(mat1.get_rawDataPointer() && mat2.get_rawDataPointer() &&
      mat1.get_size(0) == mat2.get_size(0) &&
      mat1.get_size(1) == mat2.get_size(1)) {
        return true;
    }

    return false;
  } // AreMatricesEqual_Size()

  template<typename T> bool AreMatricesEqual_SizeAndType(const Array2dUnmanaged<T> &mat1, const Array2dUnmanaged<T> &mat2)
  {
    if(mat1.get_rawDataPointer() && mat2.get_rawDataPointer() &&
      mat1.get_size(0) == mat2.get_size(0) &&
      mat1.get_size(1) == mat2.get_size(1)) {
        return true;
    }

    return false;
  } // AreMatricesEqual_SizeAndType()

  // Factory method to create an Array2dUnmanaged from the heap. The data of the returned Array2dUnmanaged must be freed by the user.
  // This is seperate from the normal constructor, as Array2dUnmanaged objects are not supposed to manage memory
  template<typename T> Array2dUnmanaged<T> AllocateArray2dUnmanagedFromHeap(s32 numRows, s32 numCols, bool useBoundaryFillPatterns=false)
  {
    // const s32 stride = Anki::Array2dUnmanaged<T>::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
    const s32 requiredMemory = 64 + 2*Anki::MEMORY_ALIGNMENT + Anki::Array2dUnmanaged<T>::ComputeMinimumRequiredMemory(numRows, numCols, useBoundaryFillPatterns); // The required memory, plus a bit more just in case

    Array2dUnmanaged<T> mat(numRows, numCols, calloc(requiredMemory, 1), requiredMemory, useBoundaryFillPatterns);

    return mat;
  } // AllocateArray2dUnmanagedFromHeap()

  template<typename T> Array2dUnmanagedFixedPoint<T> AllocateArray2dUnmanagedFixedPointFromHeap(s32 numRows, s32 numCols, s32 numFractionalBits, bool useBoundaryFillPatterns=false)
  {
    // const s32 stride = Anki::Array2dUnmanagedFixedPoint<T>::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
    const s32 requiredMemory = 64 + 2*Anki::MEMORY_ALIGNMENT + Anki::Array2dUnmanagedFixedPoint<T>::ComputeMinimumRequiredMemory(numRows, numCols, useBoundaryFillPatterns); // The required memory, plus a bit more just in case

    Array2dUnmanagedFixedPoint<T> mat(numRows, numCols, numFractionalBits, calloc(requiredMemory, 1), requiredMemory, useBoundaryFillPatterns);

    return mat;
  }

  template<typename T> FixedLengthList<T> AllocateFixedLengthListFromHeap(s32 maximumSize, bool useBoundaryFillPatterns=false)
  {
    // const s32 stride = Anki::FixedLengthList<T>::ComputeRequiredStride(maximumSize, useBoundaryFillPatterns);
    const s32 requiredMemory = 64 + 2*Anki::MEMORY_ALIGNMENT + Anki::Array2dUnmanaged<T>::ComputeMinimumRequiredMemory(1, maximumSize, useBoundaryFillPatterns); // The required memory, plus a bit more just in case

    FixedLengthList<T> mat(maximumSize, calloc(requiredMemory, 1), requiredMemory, useBoundaryFillPatterns);

    return mat;
  } // AllocateArray2dUnmanagedFixedPointFromHeap()
} //namespace Anki

#endif // _ANKICORETECH_COMMON_MATRIX_H_