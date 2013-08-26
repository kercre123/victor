//
//  block.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/block.h"


namespace Anki {
  namespace Cozmo {
    
#pragma mark --- BlockMarker3d Implementation ---
    
    const float BlockMarker3d::Width = 25;
    
    BlockMarker3d::BlockMarker3d(const Pose3d &pose_in)
    : pose(pose_in), squareCorners(4)
    {
      // Define canonical coordinates relative to center:
      const float halfWidth = 0.5f * Width;
      
      // TODO: these may need to be re-ordered and rotated in 3d
      
      squareCorners[0].x = -halfWidth;
      squareCorners[0].y = -halfWidth;
      squareCorners[0].z = 0.f;

      squareCorners[1].x = -halfWidth;
      squareCorners[1].y =  halfWidth;
      squareCorners[1].z = 0.f;

      squareCorners[2].x =  halfWidth;
      squareCorners[2].y = -halfWidth;
      squareCorners[2].z = 0.f;

      squareCorners[3].x =  halfWidth;
      squareCorners[3].y =  halfWidth;
      squareCorners[3].z = 0.f;
      
    } // Constructor: BlockMarker3d(pose)
    
    
#pragma mark --- Block Implementation ---
    
    unsigned int Block::numBlocks = 0;
    
    Block::Block(const Type type_in, const Pose3d &pose_in)
    : type(type_in), pose(pose_in)
    {
      ++Block::numBlocks;
    }
    
    Block::~Block(void)
    {
      --Block::numBlocks;
    }
    
    unsigned int Block::get_numBlocks(void)
    {
      return Block::numBlocks;
    }
    
  } // namespace Cozmo
} // namespace Anki