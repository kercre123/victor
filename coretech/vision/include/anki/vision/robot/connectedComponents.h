/**
File: connectedComponents.h
Author: Peter Barnum
Created: 2013

Definitions of connectedComponents_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_H_
#define _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_H_

#include "anki/vision/robot/connectedComponents_declarations.h"

#include "anki/common/types.h"
#include "anki/common/robot/memory.h"
#include "anki/common/robot/fixedLengthList.h"

namespace Anki
{
  namespace Embedded
  {
    // #pragma mark --- Definitions ---

    template<typename Type> ConnectedComponentSegment<Type>::ConnectedComponentSegment()
      : id(0), xStart(-1), xEnd(-1), y(-1)
    {
    } // ConnectedComponentSegment<Type>::ConnectedComponentSegment<Type>()

    template<typename Type> ConnectedComponentSegment<Type>::ConnectedComponentSegment(const s16 xStart, const s16 xEnd, const s16 y, const Type id)
      : id(id), xStart(xStart), xEnd(xEnd), y(y)
    {
    } // ConnectedComponentSegment<Type>::ConnectedComponentSegment<Type>(const s16 xStart, const s16 xEnd, const s16 y, const u16 id)

    template<typename Type> void ConnectedComponentSegment<Type>::Print() const
    {
      CoreTechPrint("[%d: (%d->%d, %d)] ", static_cast<s32>(this->id), this->xStart, this->xEnd, this->y);
    } // void ConnectedComponentSegment<Type>::Print() const

    template<typename Type> bool ConnectedComponentSegment<Type>::operator== (const ConnectedComponentSegment<Type> &component2) const
    {
      if((this->xStart == component2.xStart) &&
        (this->xEnd == component2.xEnd) &&
        (this->y == component2.y) &&
        (this->id == component2.id)) {
          return true;
      }

      return false;
    }

    inline s64 ConnectedComponents::CompareConnectedComponentSegments(const ConnectedComponentSegment<u16> &a, const ConnectedComponentSegment<u16> &b)
    {
      // Wraps zero around to MAX_u16
      u16 idA = a.id;
      u16 idB = b.id;

      idA -= 1;
      idB -= 1;

      const s64 idDifference = static_cast<s64>(u16_MAX) * static_cast<s64>(u16_MAX) * (static_cast<s64>(idA) - static_cast<s64>(idB));
      const s64 yDifference = static_cast<s64>(u16_MAX) * (static_cast<s64>(a.y) - static_cast<s64>(b.y));
      const s64 xStartDifference = (static_cast<s64>(a.xStart) - static_cast<s64>(b.xStart));

      return idDifference + yDifference + xStartDifference;
    }

    inline const ConnectedComponentSegment<u16>* ConnectedComponents::Pointer(const s32 index) const
    {
      return components.Pointer(index);
    }

    inline const ConnectedComponentSegment<u16>& ConnectedComponents::operator[](const s32 index) const
    {
      return *components.Pointer(index);
    }
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_H_
