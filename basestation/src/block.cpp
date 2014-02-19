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
    
    const Block::BlockInfoTableEntry_t Block::BlockInfoLUT_[NUM_BLOCK_TYPES] = {
      {.name = "UNKNOWN"}
#define BLOCK_DEFINITION_MODE BLOCK_LUT_MODE
#include "anki/cozmo/basestation/BlockDefinitions.h"
    };
    
#pragma mark --- Generic Block Implementation ---
    
    void Block::AddFace(const FaceName whichFace,
                        const Vision::Marker::Code &code,
                        const float markerSize_mm)
    {
      if(whichFace >= NUM_FACES) {
        // Special case: macro-generated placeholder face 
        return;
      }
      
      Pose3d facePose;
      
      const float halfWidth  = 0.5f * this->GetWidth();   // y
      const float halfHeight = 0.5f * this->GetHeight();  // z
      const float halfDepth  = 0.5f * this->GetDepth();   // x
      
      // SetSize() should have been called already
      CORETECH_ASSERT(halfDepth > 0.f && halfHeight > 0.f && halfWidth > 0.f);
      
      // The poses here are based on the Marker's canonical pose being in the
      // X-Z plane
      switch(whichFace)
      {
        case FRONT_FACE:
          facePose = Pose3d(-M_PI_2, Z_AXIS_3D, {{-halfDepth, 0.f, 0.f}},  &pose_);
          //facePose = Pose3d(0,       Z_AXIS_3D, {{-halfDepth, 0.f, 0.f}},  &pose_);
          break;
          
        case LEFT_FACE:
          facePose = Pose3d(M_PI, Z_AXIS_3D, {{0.f, halfWidth, 0.f}},  &pose_);
          //facePose = Pose3d(-M_PI_2, Z_AXIS_3D, {{0.f, -halfWidth, 0.f}},  &pose_);
          break;
          
        case BACK_FACE:
          facePose = Pose3d(M_PI_2,    Z_AXIS_3D, {{halfDepth, 0.f, 0.f}},   &pose_);
          //facePose = Pose3d(0,    Z_AXIS_3D, {{halfDepth, 0.f, 0.f}},   &pose_);
          break;
          
        case RIGHT_FACE:
          facePose = Pose3d(0,  Z_AXIS_3D, {{0.f, -halfWidth, 0.f}},   &pose_);
          //facePose = Pose3d(M_PI_2,  Z_AXIS_3D, {{0.f, halfWidth, 0.f}},   &pose_);
          break;
          
        case TOP_FACE:
          facePose = Pose3d(-M_PI_2,  X_AXIS_3D, {{0.f, 0.f, halfHeight}},  &pose_);
          //facePose = Pose3d(M_PI_2,  Y_AXIS_3D, {{0.f, 0.f, halfHeight}},  &pose_);
          break;
          
        case BOTTOM_FACE:
          facePose = Pose3d(M_PI_2, X_AXIS_3D, {{0.f, 0.f, -halfHeight}}, &pose_);
          //facePose = Pose3d(-M_PI_2, Y_AXIS_3D, {{0.f, 0.f, -halfHeight}}, &pose_);
          break;
          
        default:
          CORETECH_THROW("Unknown block face.\n");
      }
      
      AddMarker(code, facePose, markerSize_mm);
      
    } // AddFace()
    
    //unsigned int Block::numBlocks = 0;
    
    //ObjectType_t Block::NumTypes = 0;
   
    Block::Block(const ObjectType_t type)
    : ObservableObject(type),
      color_(BlockInfoLUT_[type].color),
      size_(BlockInfoLUT_[type].size),
      name_(BlockInfoLUT_[type].name),
      blockCorners_(8)
    {
      
      //++Block::numBlocks;
      
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
      
      for(auto face : BlockInfoLUT_[type_].faces) {
        AddFace(face.whichFace, face.code, face.size);
      }
      
    } // Constructor: Block(type)
    
    
    Block::~Block(void)
    {
      //--Block::numBlocks;
    }
    
     /*
    unsigned int Block::get_numBlocks(void)
    {
      return Block::numBlocks;
    }
    */
   
#pragma mark ---  Block_Cube1x1 Implementation ---
    
    //const ObjectType_t Block_Cube1x1::BlockType = Block::NumTypes++;
    
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
    
    Block_Cube1x1::Block_Cube1x1(Block::Type type)
    : Block(type)
    {
      // The sizes specified by the block definitions should
      // agree with this being a cube (all dimensions the same)
      CORETECH_ASSERT(size_.x() == size_.y())
      CORETECH_ASSERT(size_.y() == size_.z())
    }
    
    
#pragma mark ---  Block_2x1 Implementation ---
    
    //const ObjectType_t Block_2x1::BlockType = Block::NumTypes++;
    
    const std::vector<RotationMatrix3d> Block_2x1::rotationAmbiguities_ = {
      RotationMatrix3d({1,0,0,  0,1,0,  0,0,1}),
      RotationMatrix3d({1,0,0,  0,0,1,  0,1,0})
    };
    
    std::vector<RotationMatrix3d> const& Block_2x1::GetRotationAmbiguities() const
    {
      return Block_2x1::rotationAmbiguities_;
    }
    
    Block_2x1::Block_2x1(Block::Type type)
    : Block(type)
    {
      
    }

    
  } // namespace Cozmo
} // namespace Anki