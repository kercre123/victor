#include "anki/embeddedVision.h"

namespace Anki
{
  namespace Embedded
  {
    FixedLengthList_Component1d::FixedLengthList_Component1d()
      : Array_Component1d(), capacityUsed(0)
    {
    }

    FixedLengthList_Component1d::FixedLengthList_Component1d(s32 maximumSize, void * data, s32 dataLength,
      bool useBoundaryFillPatterns)
      : Array_Component1d(1, maximumSize, data, dataLength, useBoundaryFillPatterns), capacityUsed(0)
    {
    }

    FixedLengthList_Component1d::FixedLengthList_Component1d(s32 maximumSize, MemoryStack &memory,
      bool useBoundaryFillPatterns)
      : Array_Component1d(1, maximumSize, memory, useBoundaryFillPatterns), capacityUsed(0)
    {
    }

    bool FixedLengthList_Component1d::IsValid() const
    {
      if(capacityUsed > this->get_maximumSize()) {
        return false;
      }

      return Array_Component1d::IsValid();
    }

    Result FixedLengthList_Component1d::PushBack(const Component1d &value)
    {
      if(capacityUsed >= this->get_maximumSize()) {
        return RESULT_FAIL;
      }

      *this->Pointer(capacityUsed) = value;
      capacityUsed++;

      return RESULT_OK;
    }

    Component1d FixedLengthList_Component1d::PopBack()
    {
      if(capacityUsed == 0) {
        return *this->Pointer(0);
      }

      const Component1d value = *this->Pointer(capacityUsed-1);
      capacityUsed--;

      return value;
    }

    void FixedLengthList_Component1d::Clear()
    {
      this->capacityUsed = 0;
    }

    s32 FixedLengthList_Component1d::get_maximumSize() const
    {
      return Array_Component1d::get_size(1);
    }

    s32 FixedLengthList_Component1d::get_size() const
    {
      return capacityUsed;
    }

    FixedLengthList_Component1d AllocateFixedLengthListFromHeap_Component1d(s32 maximumSize, bool useBoundaryFillPatterns)
    {
      // const s32 stride = FixedLengthList_Component1d::ComputeRequiredStride(maximumSize, useBoundaryFillPatterns);
      const s32 requiredMemory = 64 + 2*MEMORY_ALIGNMENT + Array_Component1d::ComputeMinimumRequiredMemory(1, maximumSize, useBoundaryFillPatterns); // The required memory, plus a bit more

      FixedLengthList_Component1d mat(maximumSize, calloc(requiredMemory, 1), requiredMemory, useBoundaryFillPatterns);

      return mat;
    } // AllocateArray2dFixedPointFromHeap()

    FixedLengthList_Component2d::FixedLengthList_Component2d()
      : Array_Component2d(), capacityUsed(0)
    {
    }

    FixedLengthList_Component2d::FixedLengthList_Component2d(s32 maximumSize, void * data, s32 dataLength,
      bool useBoundaryFillPatterns)
      : Array_Component2d(1, maximumSize, data, dataLength, useBoundaryFillPatterns), capacityUsed(0)
    {
    }

    FixedLengthList_Component2d::FixedLengthList_Component2d(s32 maximumSize, MemoryStack &memory,
      bool useBoundaryFillPatterns)
      : Array_Component2d(1, maximumSize, memory, useBoundaryFillPatterns), capacityUsed(0)
    {
    }

    bool FixedLengthList_Component2d::IsValid() const
    {
      if(capacityUsed > this->get_maximumSize()) {
        return false;
      }

      return Array_Component2d::IsValid();
    }

    Result FixedLengthList_Component2d::PushBack(const Component2d &value)
    {
      if(capacityUsed >= this->get_maximumSize()) {
        return RESULT_FAIL;
      }

      *this->Pointer(capacityUsed) = value;
      capacityUsed++;

      return RESULT_OK;
    }

    Component2d FixedLengthList_Component2d::PopBack()
    {
      if(capacityUsed == 0) {
        return *this->Pointer(0);
      }

      const Component2d value = *this->Pointer(capacityUsed-1);
      capacityUsed--;

      return value;
    }

    void FixedLengthList_Component2d::Clear()
    {
      this->capacityUsed = 0;
    }

    s32 FixedLengthList_Component2d::get_maximumSize() const
    {
      return Array_Component2d::get_size(1);
    }

    s32 FixedLengthList_Component2d::get_size() const
    {
      return capacityUsed;
    }

    FixedLengthList_Component2d AllocateFixedLengthListFromHeap_Component2d(s32 maximumSize, bool useBoundaryFillPatterns)
    {
      // const s32 stride = FixedLengthList_Component2d::ComputeRequiredStride(maximumSize, useBoundaryFillPatterns);
      const s32 requiredMemory = 64 + 2*MEMORY_ALIGNMENT + Array_Component2d::ComputeMinimumRequiredMemory(1, maximumSize, useBoundaryFillPatterns); // The required memory, plus a bit more

      FixedLengthList_Component2d mat(maximumSize, calloc(requiredMemory, 1), requiredMemory, useBoundaryFillPatterns);

      return mat;
    } // AllocateArray2dFixedPointFromHeap()
  } // namespace Embedded
} // namespace Anki