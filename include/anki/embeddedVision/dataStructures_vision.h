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
    // A 1d component, run-length encoded, at a given y location
    class Component1d
    {
    public:
      Component1d();

      Component1d(const s16 xStart, const s16 xEnd, const s16 y);

      void Print() const;

      s16 xStart, xEnd;
      s16 y;
    }; // class Component1d

    // A 1d component, run-length encoded piece of a 2d component
    class Component2dPiece
    {
    public:
      Component2dPiece();

      Component2dPiece(const s16 xStart, const s16 xEnd, const s16 y, const u16 id);

      void Print() const;

      s16 xStart, xEnd, y, id;
    }; // class Component2dPiece
  } // namespace Embedded
} // namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_VISION_DATASTRUCTURES_VISION_H_
