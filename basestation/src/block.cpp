//
//  block.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/common/basestation/general.h"

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
      /* Still needed??
      if(whichFace >= NUM_FACES) {
        // Special case: macro-generated placeholder face 
        return;
      }
       */
      
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
      
      Vision::KnownMarker const& marker = AddMarker(code, facePose, markerSize_mm);
      
      // Store a pointer to the marker on each face:
      markersByFace_[whichFace] = &marker;
      
      //facesWithMarkerCode_[marker.GetCode()].push_back(whichFace);
      
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
      
      markersByFace_.fill(NULL);
      
      for(auto face : BlockInfoLUT_[type_].faces) {
        AddFace(face.whichFace, face.code, face.size);
      }
      
      // Every block should at least have a front face defined in the BlockDefinitions file
      CORETECH_ASSERT(markersByFace_[FRONT_FACE] != NULL);
      
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
   
    const std::array<Point3f, 6> Block::CanonicalDockingPoints = {
      {X_AXIS_3D, -X_AXIS_3D,
       Y_AXIS_3D, -Y_AXIS_3D,
       Z_AXIS_3D, -Z_AXIS_3D}
    };
    
    bool GetPreDockPose(const Point3f& canonicalPoint,
                        const float distance_mm,
                        const Pose3d& blockPose,
                        Pose3d& preDockPose)
    {
      bool dockingPointFound = false;
      
      // Compute this point's position at this distance according to this
      // block's current pose
      Point3f dockingPt(canonicalPoint);  // start with canonical point
      dockingPt *= distance_mm;           // scale to specified distance
      dockingPt =  blockPose * dockingPt; // transform to block's pose
      
      //
      // Check if it's vertically oriented
      //
      const float DOT_TOLERANCE   = .35f;
      const float ANGLE_TOLERANCE = DEG_TO_RAD(35);
      
      // Get vector, v, from center of block to this point
      Point3f v(dockingPt);
      v -= blockPose.get_translation();
      
      // Dot product of this vector with the z axis should be near zero
      // TODO: make dot product tolerance in terms of an angle?
      if( NEAR(dot(Z_AXIS_3D, v), 0.f,  distance_mm * DOT_TOLERANCE) ) {
        
        // Rotation of block around v should be a multiple of 90 degrees
        RotationMatrix3d R(0, v);
        const float angle = std::abs(R.GetAngleDiffFrom(blockPose.get_rotationMatrix()).ToFloat());
        
        if(NEAR(angle, 0.f,             ANGLE_TOLERANCE) ||
           NEAR(angle, DEG_TO_RAD(90),  ANGLE_TOLERANCE) ||
           NEAR(angle, DEG_TO_RAD(180), ANGLE_TOLERANCE) )
        {
          dockingPt.z() = 0.f;  // Project to floor plane
          preDockPose.set_translation(dockingPt);
          preDockPose.set_rotation(atan2f(-v.y(), -v.x()), Z_AXIS_3D);
          dockingPointFound = true;
        }
      }
      
      return dockingPointFound;
    }  // GetPreDockPose()
    
    
    void Block::GetPreDockPoses(const float distance_mm,
                                std::vector<PoseMarkerPair_t>& poseMarkerPairs) const
    {
      Pose3d preDockPose;
      
      //for(auto & canonicalPoint : Block::CanonicalDockingPoints)
      for(FaceName i_face = FIRST_FACE; i_face < NUM_FACES; ++i_face)
      {
        if(GetPreDockPose(CanonicalDockingPoints[i_face], distance_mm, this->pose_, preDockPose) == true) {
          poseMarkerPairs.emplace_back(preDockPose, GetMarker(i_face));
        }
      } // for each canonical docking point
      
    } // Block::GetDockingPoses()
    
    
    void Block::GetPreDockPoses(const Vision::Marker::Code& withCode,
                                const float distance_mm,
                                std::vector<Pose3d>& points) const
    {
      Pose3d preDockPose;
      
      for(FaceName i_face = FIRST_FACE; i_face < NUM_FACES; ++i_face)
      {
        if(GetMarker(i_face).GetCode() == withCode) {
          if(GetPreDockPose(CanonicalDockingPoints[i_face], distance_mm, this->pose_, preDockPose) == true) {
            points.emplace_back(preDockPose);
          }
        }
      }
    } // Block::GetDockingPoses()
    
    
    const Block::FaceName Block::OppositeFaceLUT[Block::NUM_FACES] = {
      Block::BACK_FACE,
      Block::RIGHT_FACE,
      Block::FRONT_FACE,
      Block::LEFT_FACE,
      Block::BOTTOM_FACE,
      Block::TOP_FACE
    };
    
    // prefix operator (++fname)
    Block::FaceName& operator++(Block::FaceName& fname) {
      fname = (fname < Block::NUM_FACES) ? static_cast<Block::FaceName>( static_cast<int>(fname) + 1 ) : Block::NUM_FACES;
      return fname;
    }
    
    // postfix operator (fname++)
    Block::FaceName operator++(Block::FaceName& fname, int) {
      Block::FaceName newFname = fname;
      ++newFname;
      return newFname;
    }

    
    /*
    Block::FaceName GetOppositeFace(Block::FaceName whichFace) {
      switch(whichFace)
      {
        case Block::FRONT_FACE:
          return Block::BACK_FACE;
          
        case Block::BACK_FACE:
          return Block::FRONT_FACE;
          
        case Block::LEFT_FACE:
          return Block::RIGHT_FACE;
          
        case Block::RIGHT_FACE:
          return Block::LEFT_FACE;
          
        case Block::TOP_FACE:
          return Block::BOTTOM_FACE;
          
        case Block::BOTTOM_FACE:
          return Block::TOP_FACE;
          
        default:
          CORETECH_THROW("Unknown Block::FaceName.");
      }
    }
     */
    
    Vision::KnownMarker const& Block::GetMarker(FaceName onFace) const
    {
      const Vision::KnownMarker* markerPtr = markersByFace_[onFace];
      
      if(markerPtr == NULL) {
        if(onFace == FRONT_FACE) {
          CORETECH_THROW("A front face marker should be defined for every block.");
        }
        else if( (markerPtr = markersByFace_[OppositeFaceLUT[onFace] /*GetOppositeFace(onFace)*/]) == NULL) {
            return GetMarker(FRONT_FACE);
        }
      }
      
      return *markerPtr;
      
    } // Block::GetMarker()
    
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