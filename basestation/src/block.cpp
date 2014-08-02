//
//  block.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/common/basestation/math/linearAlgebra_impl.h"

#include "anki/vision/basestation/camera.h"

#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"


#if ANKICORETECH_USE_OPENCV
#include "opencv2/imgproc/imgproc.hpp"
#endif

namespace Anki {
  namespace Cozmo {
    
    const Block::Type Block::Type::INVALID("INVALID");
    
#define BLOCK_DEFINITION_MODE BLOCK_ENUM_VALUE_MODE
#include "anki/cozmo/basestation/BlockDefinitions.h"
        
    const std::map<ObjectType, Block::BlockInfoTableEntry_t> Block::BlockInfoLUT_ = {
#define BLOCK_DEFINITION_MODE BLOCK_LUT_MODE
#include "anki/cozmo/basestation/BlockDefinitions.h"
    };
    
    const std::map<std::string, Block::Type> Block::BlockNameToTypeMap = {
#define BLOCK_DEFINITION_MODE BLOCK_STRING_TO_TYPE_LUT_MODE
#include "anki/cozmo/basestation/BlockDefinitions.h"
    };
    
#pragma mark --- Generic Block Implementation ---
    
    void Block::AddFace(const FaceName whichFace,
                        const Vision::MarkerType &code,
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
          facePose = Pose3d(-M_PI_2, Z_AXIS_3D, {{-halfDepth, 0.f, 0.f}},  &GetPose());
          //facePose = Pose3d(0,       Z_AXIS_3D, {{-halfDepth, 0.f, 0.f}},  &pose_);
          break;
          
        case LEFT_FACE:
          facePose = Pose3d(M_PI, Z_AXIS_3D, {{0.f, halfWidth, 0.f}},  &GetPose());
          //facePose = Pose3d(-M_PI_2, Z_AXIS_3D, {{0.f, -halfWidth, 0.f}},  &pose_);
          break;
          
        case BACK_FACE:
          facePose = Pose3d(M_PI_2,    Z_AXIS_3D, {{halfDepth, 0.f, 0.f}},   &GetPose());
          //facePose = Pose3d(0,    Z_AXIS_3D, {{halfDepth, 0.f, 0.f}},   &pose_);
          break;
          
        case RIGHT_FACE:
          facePose = Pose3d(0,  Z_AXIS_3D, {{0.f, -halfWidth, 0.f}},   &GetPose());
          //facePose = Pose3d(M_PI_2,  Z_AXIS_3D, {{0.f, halfWidth, 0.f}},   &pose_);
          break;
          
        case TOP_FACE:
          facePose = Pose3d(-M_PI_2,  X_AXIS_3D, {{0.f, 0.f, halfHeight}},  &GetPose());
          //facePose = Pose3d(M_PI_2,  Y_AXIS_3D, {{0.f, 0.f, halfHeight}},  &pose_);
          break;
          
        case BOTTOM_FACE:
          facePose = Pose3d(M_PI_2, X_AXIS_3D, {{0.f, 0.f, -halfHeight}}, &GetPose());
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
    
    //ObjectType Block::NumTypes = 0;
    
    const std::array<Point3f, Block::NUM_CORNERS> Block::CanonicalCorners = {{
      Point3f(-0.5f, -0.5f,  0.5f),
      Point3f( 0.5f, -0.5f,  0.5f),
      Point3f(-0.5f, -0.5f, -0.5f),
      Point3f( 0.5f, -0.5f, -0.5f),
      Point3f(-0.5f,  0.5f,  0.5f),
      Point3f( 0.5f,  0.5f,  0.5f),
      Point3f(-0.5f,  0.5f, -0.5f),
      Point3f( 0.5f,  0.5f, -0.5f)
    }};
       
    void Block::GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const
    {
      corners.resize(NUM_CORNERS);
      for(s32 i=0; i<NUM_CORNERS; ++i) {
        // Start with (zero-centered) canonical corner
        corners[i] = Block::CanonicalCorners[i];
        // Scale to the right size
        corners[i] *= _size;
        // Move to block's current pose
        corners[i] = atPose * corners[i];
      }
    }
    
    Block::Block(const ObjectType type)
    : DockableObject(type)
    , _color(BlockInfoLUT_.at(type).color)
    , _size(BlockInfoLUT_.at(type).size)
    , _name(BlockInfoLUT_.at(type).name)
    , _vizHandle(VizManager::INVALID_HANDLE)
    {
      markersByFace_.fill(NULL);
      
      for(auto face : BlockInfoLUT_.at(type_).faces) {
        AddFace(face.whichFace, face.code, face.size);
      }
      
      // Every block should at least have a front face defined in the BlockDefinitions file
      CORETECH_ASSERT(markersByFace_[FRONT_FACE] != NULL);
      
    } // Constructor: Block(type)
    
    
    Block::Block(const Block& otherBlock)
    : Block(otherBlock.GetType()) // just create an all-new block of the same type
    {
      // Put this new block at the new at the same pose as the other block
      SetPose(otherBlock.GetPose());
      
    } // Copy Constructor: Block(otherBlock)
    
    Quad3f Block::GetBoundingQuadInPlane(const Point3f& planeNormal, const f32 padding_mm) const
    {
      return GetBoundingQuadInPlane(planeNormal, GetPose(), padding_mm);
    }
    
    
    Quad3f Block::GetBoundingQuadInPlane(const Point3f& planeNormal, const Pose3d& atPose, const f32 padding_mm) const
    {
      const RotationMatrix3d& R = atPose.GetRotationMatrix();
      const Matrix_3x3f planeProjector = GetProjectionOperator(planeNormal);
      
      // Compute a single projection and rotation operator
      const Matrix_3x3f PR(planeProjector*R);
      
      Point3f paddedSize(_size);
      paddedSize += 2.f*padding_mm;
      
      std::vector<Point3f> points;
      points.reserve(8);
      for(auto corner : Block::CanonicalCorners) {
        // Scale to the right block size
        corner *= paddedSize;
        
        // Rotate to given pose and project onto the given plane
        points.emplace_back(PR*corner);
      }
      
      // TODO: 3D bounding quad doesn't exist yet
      Quad3f boundingQuad; // = GetBoundingQuad(points);
      CORETECH_ASSERT(false);
      
      /*
      // Data structure for helping me sort 3D points by their 2D distance
      // from center in the given plane
      struct planePoint {
        
        planePoint(const Point3f& pt3d, const RotationMatrix3d& R, const Matrix_3x3f& P)
        {
          // Rotate the point
          Point3f pt3d_rotated( R*pt3d );
          
          // Project it onto the given plane (i.e. remove all variation in the
          // direction of the normal)
          pt_ = P*pt3d_rotated;
          
          length_ = pt_.length();
        }
        
        bool operator<(const planePoint& other) const {
          return this->length_ > other.length_; // sort decreasing!
        }
        
        Point3f pt_;
        f32 length_;
      }; // struct planePoint
      
      const RotationMatrix3d& R = atPose.GetRotationMatrix();
      const Matrix_3x3f planeProjector = GetProjectionOperator(planeNormal);
      
      // Choose the 4 points furthest from the center of the block (in the
      // given plane)
      std::array<planePoint,8> planeCorners = {
        planePoint(blockCorners_[LEFT_FRONT_TOP],     R, planeProjector),
        planePoint(blockCorners_[RIGHT_FRONT_TOP],    R, planeProjector),
        planePoint(blockCorners_[LEFT_FRONT_BOTTOM],  R, planeProjector),
        planePoint(blockCorners_[RIGHT_FRONT_BOTTOM], R, planeProjector),
        planePoint(blockCorners_[LEFT_BACK_TOP],      R, planeProjector),
        planePoint(blockCorners_[RIGHT_BACK_TOP],     R, planeProjector),
        planePoint(blockCorners_[LEFT_BACK_BOTTOM],   R, planeProjector),
        planePoint(blockCorners_[RIGHT_BACK_BOTTOM],  R, planeProjector)
      };

      // NOTE: Uses planePoint class's operator<, which sorts in _decreasing_ order
      // so we get the 4 points the _largest_ distance from the center, after
      // rotation is applied
      std::partial_sort(planeCorners.begin(), planeCorners.begin()+4, planeCorners.end());
      
      Quad3f boundingQuad(planeCorners[0].pt_,
                          planeCorners[1].pt_,
                          planeCorners[2].pt_,
                          planeCorners[3].pt_);
      
      boundingQuad = boundingQuad.SortCornersClockwise(planeNormal);
      */
       
      // Re-center
      boundingQuad += atPose.GetTranslation();
      
      return boundingQuad;
      
    } // GetBoundingBoxInPlane()

    
    Quad2f Block::GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm) const
    {
      const RotationMatrix3d& R = atPose.GetRotationMatrix();

      Point3f paddedSize(_size);
      paddedSize += 2.f*padding_mm;
      
      std::vector<Point2f> points;
      points.reserve(8);
      for(auto corner : Block::CanonicalCorners) {
        // Scale canonical point to correct (padded) size
        corner *= paddedSize;
        
        // Rotate to given pose
        corner = R*corner;
        
        // Project onto XY plane, i.e. just drop the Z coordinate
        points.emplace_back(corner.x(), corner.y());
      }
      
      Quad2f boundingQuad = GetBoundingQuad(points);
      
      // Re-center
      Point2f center(atPose.GetTranslation().x(), atPose.GetTranslation().y());
      boundingQuad += center;
      
      return boundingQuad;
      
    } // GetBoundingBoxXY()
    
    
    Block::~Block(void)
    {
      //--Block::numBlocks;
      EraseVisualization();
    }
    
     /*
    unsigned int Block::get_numBlocks(void)
    {
      return Block::numBlocks;
    }
    */
   
    
    f32 Block::GetDefaultPreDockDistance() const
    {
      return Block::PreDockDistance;
    }    
    
    // These should match the order in which faces are defined! (See Block constructor)
    const std::array<Point3f, 6> Block::CanonicalDockingPoints = {
      {-X_AXIS_3D,
        Y_AXIS_3D,
        X_AXIS_3D,
       -Y_AXIS_3D,
        Z_AXIS_3D,
       -Z_AXIS_3D}
    };
    
    
    void Block::GetPreDockPoses(const float distance_mm,
                                std::vector<PoseMarkerPair_t>& poseMarkerPairs,
                                const Vision::Marker::Code withCode) const
    {
      Pose3d preDockPose;
      
      for(FaceName i_face = FIRST_FACE; i_face < NUM_FACES; ++i_face)
      {
        if(withCode == Vision::Marker::ANY_CODE || GetMarker(i_face).GetCode() == withCode) {
          const Vision::KnownMarker& faceMarker = GetMarker(i_face);
          const f32 distanceForThisFace = faceMarker.GetPose().GetTranslation().Length() + distance_mm;
          if(GetPreDockPose(CanonicalDockingPoints[i_face], distanceForThisFace, preDockPose) == true) {
            poseMarkerPairs.emplace_back(preDockPose, GetMarker(i_face));
          }
        }
      } // for each canonical docking point
      
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

    void Block::Visualize()
    {
      Visualize(_color);
    }
    
    void Block::Visualize(VIZ_COLOR_ID color)
    {
      Pose3d vizPose = GetPose().GetWithRespectToOrigin();
      _vizHandle = VizManager::getInstance()->DrawCuboid(GetID().GetValue(), _size, vizPose, color);
    }
    
    void Block::EraseVisualization()
    {
      // Erase the main object
      if(_vizHandle != VizManager::INVALID_HANDLE) {
        VizManager::getInstance()->EraseVizObject(_vizHandle);
        _vizHandle = VizManager::INVALID_HANDLE;
      }
      
      // Erase the pre-dock poses
      DockableObject::EraseVisualization();
    }
    
    
    ObjectType Block::GetTypeByName(const std::string& name) {
      auto typeIter = Block::BlockNameToTypeMap.find(name);
      if(typeIter != Block::BlockNameToTypeMap.end()) {
        return typeIter->second;
      } else {
        return Block::Type::INVALID;
      }
    } // GetBlockTypeByName()
    
    
#pragma mark ---  Block_Cube1x1 Implementation ---
    
    //const ObjectType Block_Cube1x1::BlockType = Block::NumTypes++;
    
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
      CORETECH_ASSERT(_size.x() == _size.y())
      CORETECH_ASSERT(_size.y() == _size.z())
    }
    
    
#pragma mark ---  Block_2x1 Implementation ---
    
    //const ObjectType Block_2x1::BlockType = Block::NumTypes++;
    
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