//
//  block.h
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#ifndef __Products_Cozmo__block__
#define __Products_Cozmo__block__

#include "anki/cozmo/messageProtocol.h"

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/quad.h"

#include "anki/vision/basestation/marker2d.h"

namespace Anki {
  
  // Forward Declarations:
  class Camera;
  
  namespace Cozmo {
    
    // Forward Declarations:
    class Robot;

    typedef u16 BlockType;
    typedef u8  FaceType;

    //
    // BlockMarker2d Class
    //
    //   Stores the representation of an observed BlockMarker in an image.
    //   Once matched with a BlockMarker3d (below), we can compute a Block or
    //   Robot's pose in the world.
    //
    class BlockMarker2d //: public Marker2d<5,2>
    {
    public:

      static const size_t NumCodeSquares;
      
      // Constructors:
      //BlockMarker2d();
      BlockMarker2d(const BlockType blockType, const FaceType faceType,
                    const Quad2f&   corners,   const MarkerUpDirection upDirection,
                    const Radians&  headAngle, Robot& seenBy);
      
      // Decoding happens on the physical robot, so don't need these (?)
      //void encodeIDs(void);
      //void decodeIDs(const BitString& bitString);
      
      // Accessors:
      BlockType      get_blockType() const;
      FaceType       get_faceType()  const;
      Radians        get_headAngle() const;
      const Quad2f&  get_quad()      const;
      Robot&         get_seenBy()    const;
      
    protected:
      BlockType blockType;
      FaceType  faceType;
      
      Quad2f corners;
      
      Radians headAngle;
      
      // Reference to the robot that saw this marker
      Robot& seenBy;
      
    }; // class BlockMarker2d
    
    
    //
    // BlockMarker3d Class
    //
    //   Stores the representation of a physical BlockMarker on the side of a
    //   3D block in the world. Finding the transformation between the
    //   corners of the marker's fiducial square and the corners of a
    //   corresponding BlockMarker2d in an image is what allows us to determine
    //   Block and Robot poses in BlockWorld.
    //
    class BlockMarker3d
    {
    public:
      
      // Dimensions (all in millimeters):
      static const float TotalWidth;              // Fiducial square plus spacing
      static const float FiducialSquareThickness; // Thickness of the square
      static const float FiducialSpacing;         // Spacing around the square
      static const float SquareWidthOutside;
      static const float SquareWidthInside;
      static const float ReferenceWidth;     // Inside/outside width, depending on tracing algorithm
      static const float CodeSquareSize;     // Dimensino of single "bit" square
      static const float DockingDotSpacing;  // b/w center docking dots
      static const float DockingDotWidth;    // diameter of center docking dots
      
      // Constructors:
      BlockMarker3d(const BlockMarker2d &marker, const Camera &camera);
      
      BlockMarker3d(const BlockType &blockType,
                    const FaceType  &faceType,
                    const Pose3d    &poseWRTparentBlock);
      
      // Accessors:
      BlockType get_blockType() const;
      FaceType  get_faceType()  const;
      
      const Pose3d& get_pose(void) const;
      void set_pose(const Pose3d &newPose);
      
      // Get the current 3D position of the marker's square corners,
      // docking target, or docking target bounding box, given its
      // current pose, and with respect to another pose.
      void getSquareCorners(Quad3f &squareCorners,
                            const Pose3d *wrtPose = Pose3d::World) const;
      
      void getDockingTarget(Quad3f &dockingTarget,
                            const Pose3d *wrtPose = Pose3d::World) const;
      
      void getDockingBoundingBox(Quad3f &boundingBox,
                                 const Pose3d *wrtPose = Pose3d::World) const;
      
    protected:
      
      // Canonical locations of the fiducial square corners, the docking target,
      // and the docking target's bounding box. These are the same for all
      // BlockMarker3d's.  The current position of a particular instance's
      // corners, target, or bounding box is obtained using that instance's
      // getSquareCorners(), getDockingTarget(), or getDockingBoundingBox()
      // methods above.
      static const Quad3f FiducialSquare;
      static const Quad3f DockingTarget;
      static const Quad3f DockingBoundingBox;
      
      BlockType blockType;
      FaceType  faceType;
      
      Pose3d pose;
      
    }; // class BlockMarker3d
    
    
    //
    // Block Class
    //
    //   Representation of a physical Block in the world.
    //
    class Block
    {
    public:
      typedef Point3<unsigned char> Color;
      
      enum FaceName {
        FRONT_FACE  = 0,
        LEFT_FACE   = 1,
        BACK_FACE   = 2,
        RIGHT_FACE  = 3,
        TOP_FACE    = 4,
        BOTTOM_FACE = 5,
        NUM_FACES
      };
      
      // "Safe" conversion from FaceType to enum FaceName (at least in Debug mode)
      static FaceName FaceType_to_FaceName(FaceType type);
      
      enum Corners {
        LEFT_FRONT_TOP =     0,
        RIGHT_FRONT_TOP =    1,
        LEFT_FRONT_BOTTOM =  2,
        RIGHT_FRONT_BOTTOM = 3,
        LEFT_BACK_TOP =      4,
        RIGHT_BACK_TOP =     5,
        LEFT_BACK_BOTTOM =   6,
        RIGHT_BACK_BOTTOM =  7
      };
      
      Block(const BlockType type);
      ~Block();
      
      static unsigned int get_numBlocks();
      
      // Accessors:
      BlockType get_type() const;
      float     get_width() const;
      float     get_height() const;
      float     get_depth() const;
      float     get_minDim() const;
      
      const Pose3d& get_pose(void) const;
      void set_pose(const Pose3d &newPose);
      
      size_t get_numMarkers() const;
      
      const BlockMarker3d& get_faceMarker(const FaceName faceType) const;
      
    protected:
      // A counter for how many blocks are instantiated
      // (A static counter may not be the best way to do this...)
      static unsigned int numBlocks;
      
      static Color   GetColorFromType(const BlockType type);
      static Point3f GetSizeFromType(const BlockType type);
      
      BlockType type;
      Color     color;
      Point3f   size;
      
      std::vector<Point3f> blockCorners;
      
      std::vector<BlockMarker3d> markers;
      void addFace(const FaceName whichFace);
      
      Pose3d pose;
      
    }; // class Block
    
    
#pragma mark --- Inline Accessors Implementations ---
    
    //
    // BlockMarker2d:
    //
    inline BlockType BlockMarker2d::get_blockType() const
    { return this->blockType; }
    
    inline FaceType BlockMarker2d::get_faceType() const
    { return this->faceType; }
    
    inline Radians BlockMarker2d::get_headAngle() const
    { return this->headAngle; }
    
    inline const Quad2f& BlockMarker2d::get_quad() const
    { return this->corners; }
    
    inline Robot& BlockMarker2d::get_seenBy() const
    { return this->seenBy; }
    
    //
    // BlockMarker3d:
    //
    inline BlockType BlockMarker3d::get_blockType() const
    { return this->blockType; }
    
    inline FaceType  BlockMarker3d::get_faceType() const
    { return this->faceType; }
    

    //
    // Block:
    //
    inline BlockType Block::get_type() const
    { return this->type; }
    
    inline float Block::get_width() const
    { return this->size.x(); }
    
    inline float Block::get_height() const
    { return this->size.z(); }
    
    inline float Block::get_depth() const
    { return this->size.y(); }
    
    inline float Block::get_minDim() const
    {
      return std::min(this->get_width(),
                      std::min(this->get_height(), this->get_depth()));
    }
    
    inline const Pose3d& Block::get_pose(void) const
    { return this->pose; }
    
    inline void Block::set_pose(const Pose3d &newPose)
    { this->pose = newPose; }
    
    inline size_t Block::get_numMarkers() const
    { return this->markers.size(); }
    
    inline const BlockMarker3d& Block::get_faceMarker(const FaceName face) const
    { return this->markers[face]; }
    
    inline Block::FaceName Block::FaceType_to_FaceName(FaceType type)
    {
      CORETECH_ASSERT(type > 0 && type < NUM_FACES+1);
      return static_cast<FaceName>(type-1);
    }
    
  } // namespace Cozmo
} // namespace Anki

#endif // __Products_Cozmo__block__