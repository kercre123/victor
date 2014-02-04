//
//  block.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/vision/basestation/camera.h"

#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
  namespace Cozmo {
    
#pragma mark --- Block Implementation ---
   
    // Fill in the static const lookup table of block information from the
    // BlockDefinitions file, using macros.
    const Block::BlockInfoTableEntry_t Block::BlockInfoLUT_[Block::NUM_BLOCK_IDS] = {
      {.name = "UNKNOWN", .color = {0.f, 0.f, 0.f}, .size = {0.f, 0.f, 0.f}}
#define BLOCK_DEFINITION_MODE BLOCK_LUT_MODE
#include "anki/cozmo/basestation/BlockDefinitions.h"
    };
    
    unsigned int Block::numBlocks = 0;
   
    Block::Block(const ObjectID_t blockID)
    : blockCorners_(8)
    {
      ID_ = blockID;
      
      color_ = BlockInfoLUT_[ID_].color;
      size_  = BlockInfoLUT_[ID_].size;
      
      ++Block::numBlocks;
      
      const float halfWidth  = 0.5f * this->GetWidth();
      const float halfHeight = 0.5f * this->GetHeight();
      const float halfDepth  = 0.5f * this->GetDepth();
      
      // x, width:  left(-)   / right(+)
      // y, depth:  front(-)  / back(+)
      // z, height: bottom(-) / top(+)
      
      blockCorners_[LEFT_FRONT_TOP]     = {-halfWidth,-halfDepth, halfHeight};
      blockCorners_[RIGHT_FRONT_TOP]    = { halfWidth,-halfDepth, halfHeight};
      blockCorners_[LEFT_FRONT_BOTTOM]  = {-halfWidth,-halfDepth,-halfHeight};
      blockCorners_[RIGHT_FRONT_BOTTOM] = { halfWidth,-halfDepth,-halfHeight};
      blockCorners_[LEFT_BACK_TOP]      = {-halfWidth, halfDepth, halfHeight};
      blockCorners_[RIGHT_BACK_TOP]     = { halfWidth, halfDepth, halfHeight};
      blockCorners_[LEFT_BACK_BOTTOM]   = {-halfWidth, halfDepth,-halfHeight};
      blockCorners_[RIGHT_BACK_BOTTOM]  = { halfWidth, halfDepth,-halfHeight};
      

      // Add a marker for each face, using the static lookup table to get
      // the marker codes
      Pose3d facePose(0, X_AXIS_3D, {{0.f, -halfDepth, 0.f}}, &pose_);
      AddMarker(BlockInfoLUT_[ID_].faceCode[FRONT_FACE], facePose,
                BlockInfoLUT_[ID_].faceSize[FRONT_FACE]);

      facePose.set_rotation(-M_PI_2, Z_AXIS_3D);
      facePose.set_translation({{-halfWidth, 0.f, 0.f}});
      AddMarker(BlockInfoLUT_[ID_].faceCode[LEFT_FACE], facePose,
                BlockInfoLUT_[ID_].faceSize[LEFT_FACE]);
      
      facePose.set_rotation(M_PI, Z_AXIS_3D);
      facePose.set_translation({{0.f, halfDepth, 0.f}});
      AddMarker(BlockInfoLUT_[ID_].faceCode[BACK_FACE], facePose,
                BlockInfoLUT_[ID_].faceSize[BACK_FACE]);
   
      facePose.set_rotation(M_PI_2, Z_AXIS_3D);
      facePose.set_translation({{halfWidth, 0.f, 0.f}});
      AddMarker(BlockInfoLUT_[ID_].faceCode[RIGHT_FACE], facePose,
                BlockInfoLUT_[ID_].faceSize[RIGHT_FACE]);
      
      facePose.set_rotation(-M_PI_2, X_AXIS_3D);
      facePose.set_translation({{0.f, 0.f, halfHeight}});
      AddMarker(BlockInfoLUT_[ID_].faceCode[TOP_FACE], facePose,
                BlockInfoLUT_[ID_].faceSize[TOP_FACE]);
      
      facePose.set_rotation(M_PI_2, X_AXIS_3D);
      facePose.set_translation({{0.f, 0.f, -halfHeight}});
      AddMarker(BlockInfoLUT_[ID_].faceCode[BOTTOM_FACE], facePose,
                BlockInfoLUT_[ID_].faceSize[BOTTOM_FACE]);
      
    } // Constructor: Block(type)
    
    Block::Block(const Block& otherBlock)
    : Block(otherBlock.ID_)
    {
      this->pose_ = otherBlock.pose_;
    }
     
    Block::~Block(void)
    {
      --Block::numBlocks;
    }
    
    unsigned int Block::get_numBlocks(void)
    {
      return Block::numBlocks;
    }
    
    
    const std::vector<RotationMatrix3d> Block_Cube1x1::rotationAmbiguities_ = {
      RotationMatrix3d({1,0,0,  0,1,0,  0,0,1}),
      RotationMatrix3d({0,1,0,  1,0,0,  0,0,1}),
      RotationMatrix3d({0,1,0,  0,0,1,  1,0,0}),
      RotationMatrix3d({0,0,1,  0,1,0,  1,0,0}),
      RotationMatrix3d({0,0,1,  1,0,0,  0,1,0}),
      RotationMatrix3d({1,0,0,  0,0,1,  0,1,0})
    };
    
    std::vector<RotationMatrix3d> const& Block_Cube1x1::GetRotationAmbiguities() const
    {
      return Block_Cube1x1::rotationAmbiguities_;
    }
    
    const std::vector<RotationMatrix3d> Block_2x1::rotationAmbiguities_ = {
      RotationMatrix3d({1,0,0,  0,1,0,  0,0,1}),
      RotationMatrix3d({1,0,0,  0,0,1,  0,1,0})
    };
    
    std::vector<RotationMatrix3d> const& Block_2x1::GetRotationAmbiguities() const
    {
      return Block_2x1::rotationAmbiguities_;
    }
    
    Block_Cube1x1::Block_Cube1x1(const ObjectID_t ID)
    : Block(ID)
    {
      
    }
    
    Block_2x1::Block_2x1(const ObjectID_t ID)
    : Block(ID)
    {
      
    }

    
  } // namespace Cozmo
} // namespace Anki