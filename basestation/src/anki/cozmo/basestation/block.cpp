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

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/imgproc/imgproc.hpp"
#endif

#define SAVE_SET_BLOCK_LIGHTS_MESSAGES_FOR_DEBUG 0

#if SAVE_SET_BLOCK_LIGHTS_MESSAGES_FOR_DEBUG
#  include <fstream>
#endif
#include <iomanip>

namespace Anki {
namespace Cozmo {

  // === Block predock pose params ===
  // {angle, x, y}
  // angle: angle about z-axis (which runs vertically along marker)
  //     x: distance along marker horizontal
  //     y: distance along marker normal
  const Pose2d BLOCK_PREDOCK_POSE_OFFSETS[] = {{0, 0, DEFAULT_PREDOCK_POSE_DISTANCE_MM}
                                               //,{0, 0, 0.8f * DEFAULT_PREDOCK_POSE_DISTANCE_MM}
                                               //,{0, 0, 0.6f * DEFAULT_PREDOCK_POSE_DISTANCE_MM}
                                               //,{0.2f, 12, DEFAULT_PREDOCK_POSE_DISTANCE_MM}
                                               //,{-0.2f, -12, DEFAULT_PREDOCK_POSE_DISTANCE_MM}
  };

  
  //#define BLOCK_DEFINITION_MODE BLOCK_ENUM_VALUE_MODE
  //#include "anki/cozmo/basestation/BlockDefinitions.h"
  
  // Static helper for looking up block properties by type
  const Block::BlockInfoTableEntry_t& Block::LookupBlockInfo(const ObjectType type)
  {
    static const std::map<ObjectType, Block::BlockInfoTableEntry_t> BlockInfoLUT = {
#       define BLOCK_DEFINITION_MODE BLOCK_LUT_MODE
#       include "anki/cozmo/basestation/BlockDefinitions.h"
  };
    return BlockInfoLUT.at(type);
  }

  
#pragma mark --- Generic Block Implementation ---
  
  void Block::AddFace(const FaceName whichFace,
                      const Vision::MarkerType &code,
                      const float markerSize_mm,
                      const u8 dockOrientations,
                      const u8 rollOrientations)
  {
    /* Still needed??
    if(whichFace >= NUM_FACES) {
      // Special case: macro-generated placeholder face 
      return;
    }
     */
    
    Pose3d facePose;
    
    const float halfWidth  = 0.5f * GetSize().y();   // y
    const float halfHeight = 0.5f * GetSize().z();  // z
    const float halfDepth  = 0.5f * GetSize().x();   // x
    
    // SetSize() should have been called already
    CORETECH_ASSERT(halfDepth > 0.f && halfHeight > 0.f && halfWidth > 0.f);
    
    // The poses here are based on the Marker's canonical pose being in the
    // X-Z plane
    switch(whichFace)
    {
      case FRONT_FACE:
        facePose = Pose3d(-M_PI_2, Z_AXIS_3D(), {{-halfDepth, 0.f, 0.f}},  &GetPose());
        //facePose = Pose3d(0,       Z_AXIS_3D(), {{-halfDepth, 0.f, 0.f}},  &pose_);
        break;
        
      case LEFT_FACE:
        facePose = Pose3d(M_PI, Z_AXIS_3D(), {{0.f, halfWidth, 0.f}},  &GetPose());
        //facePose = Pose3d(-M_PI_2, Z_AXIS_3D(), {{0.f, -halfWidth, 0.f}},  &pose_);
        break;
        
      case BACK_FACE:
        facePose = Pose3d(M_PI_2,    Z_AXIS_3D(), {{halfDepth, 0.f, 0.f}},   &GetPose());
        //facePose = Pose3d(0,    Z_AXIS_3D(), {{halfDepth, 0.f, 0.f}},   &pose_);
        break;
        
      case RIGHT_FACE:
        facePose = Pose3d(0,  Z_AXIS_3D(), {{0.f, -halfWidth, 0.f}},   &GetPose());
        //facePose = Pose3d(M_PI_2,  Z_AXIS_3D(), {{0.f, halfWidth, 0.f}},   &pose_);
        break;
        
      case TOP_FACE:
        facePose = Pose3d(-M_PI_2,  X_AXIS_3D(), {{0.f, 0.f, halfHeight}},  &GetPose());
        //facePose = Pose3d(M_PI_2,  Y_AXIS_3D(), {{0.f, 0.f, halfHeight}},  &pose_);
        break;
        
      case BOTTOM_FACE:
        facePose = Pose3d(M_PI_2, X_AXIS_3D(), {{0.f, 0.f, -halfHeight}}, &GetPose());
        //facePose = Pose3d(-M_PI_2, Y_AXIS_3D(), {{0.f, 0.f, -halfHeight}}, &pose_);
        break;
        
      default:
        CORETECH_THROW("Unknown block face.\n");
    }
    
    const Vision::KnownMarker* marker = &AddMarker(code, facePose, markerSize_mm);
    
    // NOTE: these preaction poses are really only valid for cube blocks!!!

    // The four rotation vectors for the pre-action poses created below
    const std::array<RotationVector3d,4> preActionPoseRotations = {{
      {0.f, Y_AXIS_3D()},  {M_PI_2, Y_AXIS_3D()},  {M_PI, Y_AXIS_3D()},  {-M_PI_2, Y_AXIS_3D()}
    }};
    
    // Add a pre-dock pose to each face, at fixed distance normal to the face,
    // and one for each orientation of the block
    for (u8 rot = 0; rot < 4; ++rot) {
      
      auto const& Rvec = preActionPoseRotations[rot];
      
      // Add docking preaction poses
      if (dockOrientations & (1 << rot)) {
        for (auto v : BLOCK_PREDOCK_POSE_OFFSETS) {
          Pose3d preDockPose(M_PI_2 + v.GetAngle().ToFloat(), Z_AXIS_3D(),  {{v.GetX() , -v.GetY(), -halfHeight}}, &marker->GetPose());
          preDockPose.RotateBy(Rvec);
          AddPreActionPose(PreActionPose::DOCKING, marker, preDockPose);
        }
      }
      
      // Add rolling preaction poses
      if (rollOrientations & (1 << rot)) {
        for (auto v : BLOCK_PREDOCK_POSE_OFFSETS) {
          Pose3d preDockPose(M_PI_2 + v.GetAngle().ToFloat(), Z_AXIS_3D(),  {{v.GetX() , -v.GetY(), -halfHeight}}, &marker->GetPose());
          preDockPose.RotateBy(Rvec);
          AddPreActionPose(PreActionPose::ROLLING, marker, preDockPose);
        }
      }
    }

    const f32 DefaultPrePlaceOnGroundDistance = ORIGIN_TO_LIFT_FRONT_FACE_DIST_MM - DRIVE_CENTER_OFFSET;
    for(auto const& Rvec : preActionPoseRotations) {
      
      // Add a pre-placeOnGround pose to each face, where the robot will be sitting
      // relative to the face when we put down the block -- one for each
      // orientation of the block.
      Pose3d prePlaceOnGroundPose(M_PI_2, Z_AXIS_3D(),  {{0.f, -DefaultPrePlaceOnGroundDistance, -halfHeight}}, &marker->GetPose());
      prePlaceOnGroundPose.RotateBy(Rvec);
      AddPreActionPose(PreActionPose::PLACE_ON_GROUND, marker, prePlaceOnGroundPose);
      
      // Add a pre-placeRelative pose to each face, where the robot should be before
      // it approaches the block in order to place a carried object on top of or in front of it.
      Pose3d prePlaceRelativePose(M_PI_2, Z_AXIS_3D(),  {{0.f, -DEFAULT_PREDOCK_POSE_DISTANCE_MM, -halfHeight}}, &marker->GetPose());
      prePlaceRelativePose.RotateBy(Rvec);
      AddPreActionPose(PreActionPose::PLACE_RELATIVE, marker, prePlaceRelativePose);
    }
  
  
    // Store a pointer to the marker on each face:
    markersByFace_[whichFace] = marker;
    
    //facesWithMarkerCode_[marker.GetCode()].push_back(whichFace);
    
  } // AddFace()
  
  Block::Block(const ObjectType type)
  : Block(ObjectFamily::Block, type)
  {
    
  }
  
  Block::Block(const ObjectFamily family, const ObjectType type)
  : ActionableObject(family, type)
  , _size(LookupBlockInfo(_type).size)
  , _name(LookupBlockInfo(_type).name)
  , _vizHandle(VizManager::INVALID_HANDLE)
  {
    SetColor(LookupBlockInfo(_type).color);
             
    markersByFace_.fill(NULL);
    
    for(auto face : LookupBlockInfo(_type).faces) {
      AddFace(face.whichFace, face.code, face.size, face.dockOrientations, face.rollOrientations);
    }
    
    // Every block should at least have a front face defined in the BlockDefinitions file
    CORETECH_ASSERT(markersByFace_[FRONT_FACE] != NULL);
    
  } // Constructor: Block(type)
  
  
  const std::vector<Point3f>& Block::GetCanonicalCorners() const
  {
    static const std::vector<Point3f> CanonicalCorners = {{
      Point3f(-0.5f, -0.5f,  0.5f),
      Point3f( 0.5f, -0.5f,  0.5f),
      Point3f(-0.5f, -0.5f, -0.5f),
      Point3f( 0.5f, -0.5f, -0.5f),
      Point3f(-0.5f,  0.5f,  0.5f),
      Point3f( 0.5f,  0.5f,  0.5f),
      Point3f(-0.5f,  0.5f, -0.5f),
      Point3f( 0.5f,  0.5f, -0.5f)
    }};
    
    return CanonicalCorners;
  }
  
  // Override of base class method that also scales the canonical corners
  void Block::GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const
  {
    // Start with (zero-centered) canonical corners *at unit size*
    corners = GetCanonicalCorners();
    for(auto & corner : corners) {
      // Scale to the right size
      corner *= _size;
      
      // Move to block's current pose
      corner = atPose * corner;
    }
  }
  
  // Override of base class method which scales the canonical corners
  // to the block's size
  Quad2f Block::GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm) const
  {
    const std::vector<Point3f>& canonicalCorners = GetCanonicalCorners();
    
    const Pose3d atPoseWrtOrigin = atPose.GetWithRespectToOrigin();
    const RotationMatrix3d& R = atPoseWrtOrigin.GetRotationMatrix();

    Point3f paddedSize(_size);
    paddedSize += 2.f*padding_mm;
    
    std::vector<Point2f> points;
    points.reserve(canonicalCorners.size());
    for(auto corner : canonicalCorners) {
      // Scale canonical point to correct (padded) size
      corner *= paddedSize;
      
      // Rotate to given pose
      corner = R*corner;
      
      // Project onto XY plane, i.e. just drop the Z coordinate
      points.emplace_back(corner.x(), corner.y());
    }
    
    Quad2f boundingQuad = GetBoundingQuad(points);
    
    // Re-center
    Point2f center(atPoseWrtOrigin.GetTranslation().x(), atPoseWrtOrigin.GetTranslation().y());
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
 
  /*
  f32 Block::GetDefaultPreDockDistance() const
  {
    return Block::PreDockDistance;
  } 
   */
  
  
  // These should match the order in which faces are defined! (See Block constructor)
  const std::array<Point3f, 6> Block::CanonicalDockingPoints = {
    {-X_AXIS_3D(),
      Y_AXIS_3D(),
      X_AXIS_3D(),
     -Y_AXIS_3D(),
      Z_AXIS_3D(),
     -Z_AXIS_3D()}
  };
  
  /*
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
  */
  

  
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
    static const Block::FaceName OppositeFaceLUT[Block::NUM_FACES] = {
      Block::BACK_FACE,
      Block::RIGHT_FACE,
      Block::FRONT_FACE,
      Block::LEFT_FACE,
      Block::BOTTOM_FACE,
      Block::TOP_FACE
    };
    
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
  
  const Vision::KnownMarker& Block::GetTopMarker(Pose3d& topMarkerPoseWrtOrigin) const
  {
    // Compare each face's normal's dot product with the Z axis and return the
    // one that is most closely aligned.
    // TODO: Better, cheaper algorithm for finding top face?
    //const Vision::KnownMarker* topMarker = _markers.front();
    auto topMarker = _markers.begin();
    f32 maxDotProd = std::numeric_limits<f32>::min();
    //for(FaceName whichFace = FIRST_FACE; whichFace < NUM_FACES; ++whichFace) {
    for(auto marker = _markers.begin(); marker != _markers.end(); ++marker) {
      //const Vision::KnownMarker& marker = _markers[whichFace];
      Pose3d poseWrtOrigin = marker->GetPose().GetWithRespectToOrigin();
      const f32 currentDotProd = DotProduct(marker->ComputeNormal(poseWrtOrigin), Z_AXIS_3D());
      if(currentDotProd > maxDotProd) {
        //topFace = whichFace;
        topMarker = marker;
        topMarkerPoseWrtOrigin = poseWrtOrigin;
        maxDotProd = currentDotProd;
      }
    }
    
    //PRINT_INFO("ActiveCube %d's TopMarker is = %s\n", GetID().GetValue(),
    //           Vision::MarkerTypeStrings[topMarker->GetCode()]);
    
    return *topMarker;
  }
  
  Radians Block::GetTopMarkerOrientation() const
  {
    Pose3d topMarkerPose;
    GetTopMarker(topMarkerPose);
    const Radians angle( topMarkerPose.GetRotation().GetAngleAroundZaxis() );
    return angle;
  }
  
  void Block::Visualize(const ColorRGBA& color)
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
    //DockableObject::EraseVisualization();
    ActionableObject::EraseVisualization();
  }
  
  /*
  ObjectType Block::GetTypeByName(const std::string& name)
  {
    static const std::map<std::string, Block::Type> BlockNameToTypeMap =
    {
#       define BLOCK_DEFINITION_MODE BLOCK_STRING_TO_TYPE_LUT_MODE
#       include "anki/cozmo/basestation/BlockDefinitions.h"
    };
    
    auto typeIter = BlockNameToTypeMap.find(name);
    if(typeIter != BlockNameToTypeMap.end()) {
      return typeIter->second;
    } else {
      return Block::Type::INVALID;
    }
  } // GetBlockTypeByName()
  */
  
#pragma mark ---  Block_Cube1x1 Implementation ---
  
  //const ObjectType Block_Cube1x1::BlockType = Block::NumTypes++;
  
  
  
  std::vector<RotationMatrix3d> const& Block_Cube1x1::GetRotationAmbiguities() const
  {
    static const std::vector<RotationMatrix3d> RotationAmbiguities = {
      RotationMatrix3d({1,0,0,  0,1,0,  0,0,1}),
      RotationMatrix3d({0,1,0,  1,0,0,  0,0,1}),
      RotationMatrix3d({0,1,0,  0,0,1,  1,0,0}),
      RotationMatrix3d({0,0,1,  0,1,0,  1,0,0}),
      RotationMatrix3d({0,0,1,  1,0,0,  0,1,0}),
      RotationMatrix3d({1,0,0,  0,0,1,  0,1,0})
    };
    
    return RotationAmbiguities;
  }
  
#pragma mark ---  Block_2x1 Implementation ---
  
  //const ObjectType Block_2x1::BlockType = Block::NumTypes++;
  
  
  
  std::vector<RotationMatrix3d> const& Block_2x1::GetRotationAmbiguities() const
  {
    static const std::vector<RotationMatrix3d> RotationAmbiguities = {
      RotationMatrix3d({1,0,0,  0,1,0,  0,0,1}),
      RotationMatrix3d({1,0,0,  0,0,1,  0,1,0})
    };
    
    return RotationAmbiguities;
  }



} // namespace Cozmo
} // namespace Anki