//
//  block.h
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#ifndef __Products_Cozmo__block__
#define __Products_Cozmo__block__

#include "anki/math/pose.h"

#include "anki/vision/marker2d.h"

namespace Anki {
  namespace Cozmo {
    
    class BlockMarker2d : public Marker2d
    {
      
    }; // class BlockMarker2d
    
    
    class BlockMarker3d
    {
    public:
      typedef unsigned char Type;
      
      // TODO: Get/store marker Width in some other way.
      // This should probably be some kind of parameter read in
      // from a config or determined on a per-marker or per-block basis
      // in case we want to have blocks of different sizes later.
      static const float Width;
      
      BlockMarker3d(const Pose3d &poseWRTparentBlock);
      
    protected:
      std::vector<Point3f> squareCorners;
      Pose3d pose;
      
    }; // class BlockMarker3d
    
    
    class Block
    {
    public:
      typedef unsigned char Type;

      Block(const Type type, const Pose3d &pose);
      ~Block();
      
      static unsigned int get_numBlocks();
      
    protected:
      // A counter for how many blocks are instantiated
      // (A static counter may not be the best way to do this...)
      static unsigned int numBlocks;
      
      Type type;
      
      std::vector<BlockMarker3d> markers;
      
      Pose3d pose;
      
    }; // class Block
    
  } // namespace Cozmo
} // namespace Anki

#endif // __Products_Cozmo__block__