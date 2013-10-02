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

#define BLOCKMARKER3D_USE_OUTSIDE_SQUARE true

namespace Anki {
  namespace Cozmo {
    
    class BlockMarker2d : public Marker2d<5>
    {
    public:

      
    protected:
      std::vector<Point2f> squareCorners;
      Pose2d pose;
      
      using Marker2d<5>::Layout;
      
    }; // class BlockMarker2d
    
    
    class BlockMarker3d
    {
    public:
      typedef unsigned char Type;
      
      // Dimensions (all in millimeters):
      static const float TotalWidth;
      static const float FiducialSquareWidth;
      static const float FiducialSpacing;     // spacing around fiducial square
      static const float SquareWidthOutside;
      static const float SquareWidthInside;
      static const float ReferenceWidth;
      static const float CodeSquareWidth;     // single "bit" square
      static const float DockingDotSpacing;   // b/w center docking dots
      static const float DockingDotWidth;     // diaemter of center docking dots
      
      // Constructors:
      BlockMarker3d(const Pose3d &poseWRTparentBlock);
      
      // Accessors:
      const Pose3d& get_pose(void) const;
      void set_pose(const Pose3d &newPose);
      
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
      
      // Accessors:
      const Pose3d& get_pose(void) const;
      void set_pose(const Pose3d &newPose);
      
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