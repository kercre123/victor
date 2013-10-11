//
//  block.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/block.h"
#include "anki/cozmo/robot.h"
#include "anki/vision/camera.h"

namespace Anki {
  namespace Cozmo {

#pragma mark --- BlockMarker2d Dimensions & Layout---
    
    
#pragma mark --- BlockMarker2d Implementations ---
    const size_t BlockMarker2d::NumCodeSquares = 5;
    
    BlockMarker2d::BlockMarker2d(BlockType    blockTypeIn,
                                 FaceType     faceTypeIn,
                                 const Quad2f &cornersIn)
    : blockType(blockTypeIn), faceType(faceTypeIn), corners(cornersIn)
    {
      
    }
    
#pragma mark --- BlockMarker3d Dimensions & Layout---
    
    
    const float BlockMarker3d::TotalWidth          = 38.f;
    const float BlockMarker3d::FiducialSquareThickness = 4.f;
    const float BlockMarker3d::FiducialSpacing     = 3.f;
    
    const float BlockMarker3d::SquareWidthOutside =
    (BlockMarker3d::TotalWidth - 2.f*BlockMarker3d::FiducialSpacing);
    
    const float BlockMarker3d::SquareWidthInside =
    (BlockMarker3d::SquareWidthOutside - 2.f*BlockMarker3d::FiducialSquareThickness);
    
#if(BLOCKMARKER3D_USE_OUTSIDE_SQUARE)
    const float BlockMarker3d::ReferenceWidth = BlockMarker3d::SquareWidthOutside;
#else
    const float BlockMarker3d::ReferenceWidth = BlockMarker3d::SquareWidthInside;
#endif
    
    const float BlockMarker3d::CodeSquareSize =
    (BlockMarker3d::TotalWidth - 4.f*BlockMarker3d::FiducialSpacing -
     2.f*BlockMarker3d::FiducialSquareThickness) /
    float(BlockMarker2d::NumCodeSquares);
    
    const float BlockMarker3d::DockingDotSpacing =
    BlockMarker3d::CodeSquareSize/2.f;
    
    const float BlockMarker3d::DockingDotWidth =
    BlockMarker3d::DockingDotSpacing/2.f;
    
    
    Quad3f setCanonicalSquareCorners(const float w)
    {
      return Quad3f({-w, 0.f,  w},  // TopLeft
                    {-w, 0.f, -w},  // BottomLeft
                    { w, 0.f,  w},  // TopRight
                    { w, 0.f, -w}); // BottomRight
    }
    
    const Quad3f BlockMarker3d::FiducialSquare =
    setCanonicalSquareCorners(0.5f * BlockMarker3d::ReferenceWidth);
    
    const Quad3f BlockMarker3d::DockingTarget =
    setCanonicalSquareCorners(0.5f * BlockMarker3d::DockingDotSpacing);
    
    const Quad3f BlockMarker3d::DockingBoundingBox =
    setCanonicalSquareCorners(0.5f * BlockMarker3d::CodeSquareSize);
    
    
#pragma mark --- BlockMarker3d Implementation ---
    
    BlockMarker3d::BlockMarker3d(const BlockMarker2d &marker,
                                 const Camera        &camera)
    : blockType(marker.get_blockType()),
      faceType(marker.get_faceType()),
      pose(camera.computeObjectPose(marker.get_quad(),
                                    BlockMarker3d::FiducialSquare))
    {

    } // Constructor: BlockMarker3d(marker2d, camera)
    
    
    BlockMarker3d::BlockMarker3d(const BlockType &blockType_in,
                                 const FaceType  &faceType_in,
                                 const Pose3d    &pose_in)
    : blockType(blockType_in), faceType(faceType_in), pose(pose_in)
    {
      
      
    } // Constructor: BlockMarker3d(blockType,FaceType,pose)
    
    void BlockMarker3d::getSquareCorners(std::vector<Point3f> &squareCorners,
                                         const Pose3d *wrtPose) const
    {
      Pose3d P = this->pose.getWithRespectTo(wrtPose);
      P.applyTo(BlockMarker3d::FiducialSquare, squareCorners);
    }
    
    void BlockMarker3d::getDockingTarget(std::vector<Point3f> &dockingTarget,
                                         const Pose3d *wrtPose) const
    {
      Pose3d P = this->pose.getWithRespectTo(wrtPose);
      P.applyTo(BlockMarker3d::DockingTarget, dockingTarget);
    }
    
    void BlockMarker3d::getDockingBoundingBox(std::vector<Point3f> &boundingBox,
                                              const Pose3d *wrtPose) const
    {
      Pose3d P = this->pose.getWithRespectTo(wrtPose);
      P.applyTo(BlockMarker3d::DockingBoundingBox, boundingBox);
    }
    
    
#pragma mark --- Block Implementation ---
    
    unsigned int Block::numBlocks = 0;
    
    Block::Color Block::GetColorFromType(const BlockType type)
    {
      // TODO: read block colors from some kind of definition file
      
      switch(type)
      {
        case 60: // Webot Simulated Red 1x1 Block
          return Block::Color(255, 0, 0);
          
        case 65: // Webot Simulated Green 1x1 Block
          return Block::Color(0, 255, 0);
          
        case 70: // Webot Simulated Blue 1x1 Block
          return Block::Color(0, 0, 255);
          
        case 75: // Webot Simulated Purple 2x1 Block
          return Block::Color(128, 26, 191);
          
        default:
          // TODO: Handle unrecognized blocks more gracefully
          CORETECH_THROW("Unrecognized Block Type");
      }
      
      return Block::Color(0, 0, 0);
      
    } // GetColorFromType()
    
    Point3f Block::GetSizeFromType(const BlockType type)
    {
      // TODO: read block dimensions from some kind of definition file
      
      switch(type)
      {

        case 60:
        case 65:
        case 70:
          // Webot Simulated 1x1 Blocks:
          return Point3f(60.f, 60.f, 60.f);
          
        case 75:
          // Webot Simulated Purple 2x1 Block
          return Point3f(128, 26, 191);
          
        default:
          // TODO: Handle unrecognized blocks more gracefully
          CORETECH_THROW("Unrecognized Block Type");
      }
      
      return Point3f(0, 0, 0);
      
    } // GetSizeFromType()
    
  
    Block::Block(const BlockType type_in)
    : type(type_in),
      color(Block::GetColorFromType(type)),
      size(Block::GetSizeFromType(type)),
      blockCorners(8)
    {
      ++Block::numBlocks;
      
      const float halfWidth  = 0.5f * this->get_width();
      const float halfHeight = 0.5f * this->get_height();
      const float halfDepth  = 0.5f * this->get_depth();
      
      // x, width:  left(-)   / right(+)
      // y, depth:  front(-)  / back(+)
      // z, height: bottom(-) / top(+)
      
      this->blockCorners[LEFT_FRONT_TOP]     = {-halfWidth,-halfDepth, halfHeight};
      this->blockCorners[RIGHT_FRONT_TOP]    = { halfWidth,-halfDepth, halfHeight};
      this->blockCorners[LEFT_FRONT_BOTTOM]  = {-halfWidth,-halfDepth,-halfHeight};
      this->blockCorners[RIGHT_FRONT_BOTTOM] = { halfWidth,-halfDepth,-halfHeight};
      this->blockCorners[LEFT_BACK_TOP]      = {-halfWidth, halfDepth, halfHeight};
      this->blockCorners[RIGHT_BACK_TOP]     = { halfWidth, halfDepth, halfHeight};
      this->blockCorners[LEFT_BACK_BOTTOM]   = {-halfWidth, halfDepth,-halfHeight};
      this->blockCorners[RIGHT_BACK_BOTTOM]  = { halfWidth, halfDepth,-halfHeight};
      
      // Create a block marker on each face:
      markers.resize(6, NULL);
      
      const Vec3f Xaxis(1.f, 0.f, 0.f), Zaxis(0.f, 0.f, 1.f);
      
      Pose3d facePose(0, Zaxis, {{0.f, -halfDepth, 0.f}}, &(this->pose));
      markers[FRONT_FACE] = new BlockMarker3d(this->type, FRONT_FACE, facePose);
      
      facePose.set_rotation(M_PI, Zaxis);
      facePose.set_translation({{0.f, halfDepth, 0.f}});
      markers[BACK_FACE] = new BlockMarker3d(this->type, BACK_FACE, facePose);
   
      facePose.set_rotation(-M_PI_2, Zaxis);
      facePose.set_translation({{-halfWidth, 0.f, 0.f}});
      markers[LEFT_FACE] = new BlockMarker3d(this->type, LEFT_FACE, facePose);
      
      facePose.set_rotation(M_PI_2, Zaxis);
      facePose.set_translation({{halfWidth, 0.f, 0.f}});
      markers[RIGHT_FACE] = new BlockMarker3d(this->type, RIGHT_FACE, facePose);
      
      facePose.set_rotation(-M_PI_2, Xaxis);
      facePose.set_translation({{0.f, 0.f, halfHeight}});
      markers[TOP_FACE] = new BlockMarker3d(this->type, TOP_FACE, facePose);
      
      facePose.set_rotation(M_PI_2, Xaxis);
      facePose.set_translation({{0.f, 0.f, -halfHeight}});
      markers[BOTTOM_FACE] = new BlockMarker3d(this->type, BOTTOM_FACE, facePose);
      
    } // Constructor: Block(type)
    
     
    Block::~Block(void)
    {
      --Block::numBlocks;
      
      for(BlockMarker3d* marker : this->markers)
      {
        CORETECH_ASSERT(marker != NULL);
        
        delete marker;
      }
    }
    
    unsigned int Block::get_numBlocks(void)
    {
      return Block::numBlocks;
    }
    
  } // namespace Cozmo
} // namespace Anki