//
//  dataStructures.h
//  CoreTech_Common
//
//  Created by Peter Barnum on 8/7/13.
//  Copyright (c) 2013 Peter Barnum. All rights reserved.
//

#ifndef _ANKICORETECHEMBEDDED_COMMON_FIXED_LENGTH_LIST_H_
#define _ANKICORETECHEMBEDDED_COMMON_FIXED_LENGTH_LIST_H_

#include "anki/embeddedCommon/config.h"

namespace Anki
{
  namespace Embedded
  {
    class FixedLengthList_Point_s16 : public Array_Point_s16
    {
    public:
      FixedLengthList_Point_s16();

      // Constructor for a FixedLengthList_Point_s16, pointing to user-allocated data.
      FixedLengthList_Point_s16(s32 maximumSize, void * data, s32 dataLength, bool useBoundaryFillPatterns=false);

      // Constructor for a FixedLengthList_Point_s16, pointing to user-allocated MemoryStack
      FixedLengthList_Point_s16(s32 maximumSize, MemoryStack &memory, bool useBoundaryFillPatterns=false);

      bool IsValid() const;

      Result PushBack(const Point_s16 &value);

      // Will act as a normal pop, except when the list is empty. Then subsequent
      // calls will keep returning the first value in the list.
      Point_s16 PopBack();

      void Clear();

      // Pointer to the data, at a given location
      inline Point_s16* Pointer(s32 index);

      // Pointer to the data, at a given location
      inline const Point_s16* Pointer(s32 index) const;

      s32 get_maximumSize() const;

      s32 get_size() const;

    protected:
      s32 capacityUsed;
    }; // class FixedLengthList_Point_s16

    inline Point_s16* FixedLengthList_Point_s16::Pointer(s32 index)
    {
      return Array_Point_s16::Pointer(0, index);
    }

    // Pointer to the data, at a given location

    inline const Point_s16* FixedLengthList_Point_s16::Pointer(s32 index) const
    {
      return Array_Point_s16::Pointer(0, index);
    }
  } // namespace Embedded
} // namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_FIXED_LENGTH_LIST_H_