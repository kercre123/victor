//
//  dataStructures.h
//  CoreTech_Common
//
//  Created by Peter Barnum on 8/7/13.
//  Copyright (c) 2013 Peter Barnum. All rights reserved.
//

#ifndef _ANKICORETECHEMBEDDED_VISION_DATASTRUCTURES_VISION_H_
#define _ANKICORETECHEMBEDDED_VISION_DATASTRUCTURES_VISION_H_

#include "anki/embeddedVision/config.h"

namespace Anki
{
  namespace Embedded
  {
    class Component1d
    {
    public:
      Component1d();

      Component1d(const s16 xStart, const s16 xEnd);

      void Print() const;

      s16 xStart, xEnd;
    }; // class Component1d

    class Component2d
    {
    public:
      Component2d();

      Component2d(const s16 xStart, const s16 xEnd, const s16 y, const u16 id);

      void Print() const;

      s16 xStart, xEnd, y, id;
    }; // class Component2d
  } // namespace Embedded
} // namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_VISION_DATASTRUCTURES_VISION_H_
