#include "anki/embeddedCommon.h"

namespace Anki
{
  namespace Embedded
  {
    FixedLengthList_Point_s16::FixedLengthList_Point_s16()
      : Array_Point_s16(), capacityUsed(0)
    {
    }

    FixedLengthList_Point_s16::FixedLengthList_Point_s16(s32 maximumSize, void * data, s32 dataLength,
      bool useBoundaryFillPatterns)
      : Array_Point_s16(1, maximumSize, data, dataLength, useBoundaryFillPatterns), capacityUsed(0)
    {
    }

    FixedLengthList_Point_s16::FixedLengthList_Point_s16(s32 maximumSize, MemoryStack &memory,
      bool useBoundaryFillPatterns)
      : Array_Point_s16(1, maximumSize, memory, useBoundaryFillPatterns), capacityUsed(0)
    {
    }

    bool FixedLengthList_Point_s16::IsValid() const
    {
      if(capacityUsed > this->get_maximumSize()) {
        return false;
      }

      return Array_Point_s16::IsValid();
    }

    Result FixedLengthList_Point_s16::PushBack(const Point_s16 &value)
    {
      if(capacityUsed >= this->get_maximumSize()) {
        return RESULT_FAIL;
      }

      *this->Pointer(capacityUsed) = value;
      capacityUsed++;

      return RESULT_OK;
    }

    Point_s16 FixedLengthList_Point_s16::PopBack()
    {
      if(capacityUsed == 0) {
        return *this->Pointer(0);
      }

      const Point_s16 value = *this->Pointer(capacityUsed-1);
      capacityUsed--;

      return value;
    }

    void FixedLengthList_Point_s16::Clear()
    {
      this->capacityUsed = 0;
    }

    s32 FixedLengthList_Point_s16::get_maximumSize() const
    {
      return Array_Point_s16::get_size(1);
    }

    s32 FixedLengthList_Point_s16::get_size() const
    {
      return capacityUsed;
    }
  } // namespace Embedded
} // namespace Anki