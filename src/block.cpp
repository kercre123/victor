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

#pragma mark --- BlockMarker2d Dimensions & Layout---
    
    const char BlockMarker2d::Layout[25] = {'A', 'B', 'C'};
    
#pragma mark --- BlockMarker3d Dimensions & Layout---
    
    
    const float BlockMarker3d::TotalWidth          = 38.f;
    const float BlockMarker3d::FiducialSquareWidth = 4.f;
    const float BlockMarker3d::FiducialSpacing     = 3.f;
    
    const float BlockMarker3d::SquareWidthOutside =
    (BlockMarker3d::TotalWidth - 2.f*BlockMarker3d::FiducialSpacing);
    
    const float BlockMarker3d::SquareWidthInside =
    (BlockMarker3d::SquareWidthOutside - 2.f*BlockMarker3d::FiducialSquareWidth);
    
#if(BLOCKMARKER3D_USE_OUTSIDE_SQUARE)
    const float BlockMarker3d::ReferenceWidth = BlockMarker3d::SquareWidthOutside;
#else
    const float BlockMarker3d::ReferenceWidth = BlockMarker3d::SquareWidthInside;
#endif
    
    const float BlockMarker3d::CodeSquareWidth =
    (BlockMarker3d::TotalWidth - 4.f*BlockMarker3d::FiducialSpacing -
     2.f*BlockMarker3d::FiducialSquareWidth) / BlockMarker2d::NumCodeSquaresPerSide;
    
    const float BlockMarker3d::DockingDotSpacing =
    BlockMarker3d::CodeSquareWidth/2.f;
    
    const float BlockMarker3d::DockingDotWidth =
    BlockMarker3d::DockingDotSpacing/2.f;
    
    
#pragma mark --- BlockMarker3d Implementation ---

    
    BlockMarker3d::BlockMarker3d(const Pose3d &pose_in)
    : squareCorners(4), pose(pose_in)
    {
      // Define canonical coordinates relative to center:
      const float halfWidth = 0.5f * BlockMarker3d::ReferenceWidth;
      
      // TODO: these may need to be re-ordered and rotated in 3d
      
      
      // Define canonical corner locations:
      squareCorners[BlockMarker2d::TopLeftCorner].x()     = -halfWidth;
      squareCorners[BlockMarker2d::TopLeftCorner].y()     = -halfWidth;
      squareCorners[BlockMarker2d::TopLeftCorner].z()     =  0.f;

      squareCorners[BlockMarker2d::BottomLeftCorner].x()  = -halfWidth;
      squareCorners[BlockMarker2d::BottomLeftCorner].y()  =  halfWidth;
      squareCorners[BlockMarker2d::BottomLeftCorner].z()  =  0.f;

      squareCorners[BlockMarker2d::TopRightCorner].x()    =  halfWidth;
      squareCorners[BlockMarker2d::TopRightCorner].y()    = -halfWidth;
      squareCorners[BlockMarker2d::TopRightCorner].z()    =  0.f;

      squareCorners[BlockMarker2d::BottomRightCorner].x() =  halfWidth;
      squareCorners[BlockMarker2d::BottomRightCorner].y() =  halfWidth;
      squareCorners[BlockMarker2d::BottomRightCorner].z() =  0.f;
      
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