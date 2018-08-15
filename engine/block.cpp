//
//  block.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "coretech/common/engine/math/linearAlgebra_impl.h"

#include "coretech/vision/engine/camera.h"

#include "engine/block.h"
#include "engine/robot.h"

#include "coretech/common/engine/math/quad_impl.h"

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
namespace Vector {

  // === Block predock pose params ===
  // {angle, x, y}
  // angle: angle about z-axis (which runs vertically along marker)
  //     x: distance along marker horizontal
  //     y: distance along marker normal
  const Pose2d kBlockPreDockPoseOffset = {0, 0, DEFAULT_MIN_PREDOCK_POSE_DISTANCE_MM};

  
  //#define BLOCK_DEFINITION_MODE BLOCK_ENUM_VALUE_MODE
  //#include "engine/BlockDefinitions.h"
  
  // Static helper for looking up block properties by type
  const Block::BlockInfoTableEntry_t& Block::LookupBlockInfo(const ObjectType type)
  {
    static const std::map<ObjectType, Block::BlockInfoTableEntry_t> BlockInfoLUT = {
#     define BLOCK_DEFINITION_MODE BLOCK_LUT_MODE
#     include "engine/BlockDefinitions.h"
    };
    
    // If this assertion fails, somebody is trying to construct an invalid
    // block type
    auto entry = BlockInfoLUT.find(type);
    DEV_ASSERT(entry != BlockInfoLUT.end(), "Block.LookupBlockInfo.InvalidBlockType");
    return entry->second;
  }

  
#pragma mark --- Generic Block Implementation ---
  
  void Block::AddFace(const FaceName whichFace,
                      const Vision::MarkerType &code,
                      const float markerSize_mm,
                      const u8 dockOrientations,
                      const u8 rollOrientations)
  {
    Pose3d facePose;
    
    const float halfWidth  = 0.5f * GetSize().y();  
    const float halfHeight = 0.5f * GetSize().z();  
    const float halfDepth  = 0.5f * GetSize().x();
    
    // SetSize() should have been called already
    DEV_ASSERT(halfDepth > 0.f && halfHeight > 0.f && halfWidth > 0.f, "Block.AddFace.InvalidHalfSize");
    
    // The poses here are based on the Marker's canonical pose being in the
    // X-Z plane
    // NOTE: these poses intentionally have no parent. That is handled by AddMarker below.
    switch(whichFace)
    {
      case FRONT_FACE:
        facePose = Pose3d(-M_PI_2_F, Z_AXIS_3D(), {-halfDepth, 0.f, 0.f});
        break;
        
      case LEFT_FACE:
        facePose = Pose3d(M_PI,    Z_AXIS_3D(), {0.f, halfWidth, 0.f});
        break;
        
      case BACK_FACE:
        facePose = Pose3d(M_PI_2,  Z_AXIS_3D(), {halfDepth, 0.f, 0.f});
        break;
        
      case RIGHT_FACE:
        facePose = Pose3d(0,       Z_AXIS_3D(), {0.f, -halfWidth, 0.f});
        break;
        
      case TOP_FACE:
        // Rotate -90deg around X, then -90 around Z
        facePose = Pose3d(2.09439510f, {-0.57735027f, 0.57735027f, -0.57735027f}, {0.f, 0.f, halfHeight});
        break;
        
      case BOTTOM_FACE:
        // Rotate +90deg around X, then -90 around Z
        facePose = Pose3d(2.09439510f, {0.57735027f, -0.57735027f, -0.57735027f}, {0.f, 0.f, -halfHeight});
        break;
        
      default:
        CORETECH_THROW("Unknown block face.\n");
    }
    
    // Store a pointer to the marker on each face:
    markersByFace_[whichFace] = &AddMarker(code, facePose, markerSize_mm);
    
  } // AddFace()
  
  
  void Block::GeneratePreActionPoses(const PreActionPose::ActionType type,
                                     std::vector<PreActionPose>& preActionPoses) const
  {
    preActionPoses.clear();
    
    const float halfWidth  = 0.5f * GetSize().y();
    const float halfHeight = 0.5f * GetSize().z();
    
    // The four rotation vectors for the pre-action poses created below
    static const std::array<RotationVector3d,4> kPreActionPoseRotations = {{
      {0.f, Y_AXIS_3D()},  {M_PI_2_F, Y_AXIS_3D()},  {M_PI_F, Y_AXIS_3D()},  {-M_PI_2_F, Y_AXIS_3D()}
    }};
  
    for(const auto& face : LookupBlockInfo(_type).faces)
    {
      const auto& marker = GetMarker(face.whichFace);
      
      // Add a pre-dock pose to each face, at fixed distance normal to the face,
      // and one for each orientation of the block
      for (u8 rot = 0; rot < 4; ++rot)
      {
        auto const& Rvec = kPreActionPoseRotations[rot];
        
        switch(type)
        {
          case PreActionPose::ActionType::DOCKING:
          {
            if (face.dockOrientations & (1 << rot))
            {
              Pose3d preDockPose(M_PI_2 + kBlockPreDockPoseOffset.GetAngle().ToFloat(),
                                 Z_AXIS_3D(),
                                 {kBlockPreDockPoseOffset.GetX() , -kBlockPreDockPoseOffset.GetY(), -halfHeight},
                                 marker.GetPose());
              
              preDockPose.RotateBy(Rvec);
              
              preActionPoses.emplace_back(PreActionPose::DOCKING,
                                          &marker,
                                          preDockPose,
                                          DEFAULT_PREDOCK_POSE_LINE_LENGTH_MM);
            }
            break;
          }
          case PreActionPose::ActionType::FLIPPING:
          {
            // Flip preActionPoses are at the corners of the block so need to divided by root2 to get x and y dist
            const float flipPreActionPoseDist = FLIP_PREDOCK_POSE_DISTAMCE_MM / 1.414f;
            
            if (face.dockOrientations & (1 << rot))
            {
              Pose3d preDockPose(M_PI_2 + M_PI_4 + kBlockPreDockPoseOffset.GetAngle().ToFloat(),
                                 Z_AXIS_3D(),
                                 {flipPreActionPoseDist + halfWidth, -flipPreActionPoseDist, -halfHeight},
                                 marker.GetPose());
              
              preDockPose.RotateBy(Rvec);
              
              preActionPoses.emplace_back(PreActionPose::FLIPPING,
                                          &marker,
                                          preDockPose,
                                          0);
            }
            break;
          }
          case PreActionPose::ActionType::PLACE_ON_GROUND:
          {
            const f32 DefaultPrePlaceOnGroundDistance = ORIGIN_TO_LIFT_FRONT_FACE_DIST_MM - DRIVE_CENTER_OFFSET;
            
            // Add a pre-placeOnGround pose to each face, where the robot will be sitting
            // relative to the face when we put down the block -- one for each
            // orientation of the block.
            Pose3d prePlaceOnGroundPose(M_PI_2,
                                        Z_AXIS_3D(),
                                        Point3f{0.f, -DefaultPrePlaceOnGroundDistance, -halfHeight},
                                        marker.GetPose());
            
            prePlaceOnGroundPose.RotateBy(Rvec);
            
            preActionPoses.emplace_back(PreActionPose::PLACE_ON_GROUND,
                                        &marker,
                                        prePlaceOnGroundPose,
                                        0);
            break;
          }
          case PreActionPose::ActionType::PLACE_RELATIVE:
          {
            // Add a pre-placeRelative pose to each face, where the robot should be before
            // it approaches the block in order to place a carried object on top of or in front of it.
            Pose3d prePlaceRelativePose(M_PI_2,
                                        Z_AXIS_3D(),
                                        Point3f{0.f, -PLACE_RELATIVE_MIN_PREDOCK_POSE_DISTANCE_MM, -halfHeight},
                                        marker.GetPose());
            
            prePlaceRelativePose.RotateBy(Rvec);
            
            preActionPoses.emplace_back(PreActionPose::PLACE_RELATIVE,
                                        &marker,
                                        prePlaceRelativePose,
                                        PLACE_RELATIVE_PREDOCK_POSE_LINE_LENGTH_MM);
            break;
          }
          case PreActionPose::ActionType::ROLLING:
          {
            if (face.rollOrientations & (1 << rot))
            {
              Pose3d preDockPose(M_PI_2 + kBlockPreDockPoseOffset.GetAngle().ToFloat(),
                                 Z_AXIS_3D(),
                                 {kBlockPreDockPoseOffset.GetX() , -kBlockPreDockPoseOffset.GetY(), -halfHeight},
                                 marker.GetPose());
              
              preDockPose.RotateBy(Rvec);
              
              preActionPoses.emplace_back(PreActionPose::ROLLING,
                                          &marker,
                                          preDockPose,
                                          DEFAULT_PREDOCK_POSE_LINE_LENGTH_MM);
              
            }
            break;
          }
          case PreActionPose::ActionType::NONE:
          case PreActionPose::ActionType::ENTRY:
          {
            break;
          }
        }
      }
    }
  }
  

  Block::Block(const ObjectType type)
  : Block(ObjectFamily::Block, type)
  {
    
  }
  
  Block::Block(const ObjectFamily family, const ObjectType type)
  : ObservableObject(family, type)
  , _size(LookupBlockInfo(_type).size)
  , _name(LookupBlockInfo(_type).name)
  , _vizHandle(VizManager::INVALID_HANDLE)
  {
    SetColor(LookupBlockInfo(_type).color);
             
    markersByFace_.fill(NULL);
    
    for(const auto& face : LookupBlockInfo(_type).faces) {
      AddFace(face.whichFace, face.code, face.size, face.dockOrientations, face.rollOrientations);
    }
    
    // Every block should at least have a front face defined in the BlockDefinitions file
    DEV_ASSERT(markersByFace_[FRONT_FACE] != NULL, "Block.Constructor.InvalidFrontFace");
    
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
    
    const Pose3d atPoseWrtOrigin = atPose.GetWithRespectToRoot();
    const Rotation3d& R = atPoseWrtOrigin.GetRotation();

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
    f32 maxDotProd = std::numeric_limits<f32>::lowest();
    //for(FaceName whichFace = FIRST_FACE; whichFace < NUM_FACES; ++whichFace) {
    for(auto marker = _markers.begin(); marker != _markers.end(); ++marker) {
      //const Vision::KnownMarker& marker = _markers[whichFace];
      Pose3d poseWrtOrigin = marker->GetPose().GetWithRespectToRoot();
      const f32 currentDotProd = DotProduct(marker->ComputeNormal(poseWrtOrigin), Z_AXIS_3D());
      if(currentDotProd > maxDotProd) {
        //topFace = whichFace;
        topMarker = marker;
        topMarkerPoseWrtOrigin = poseWrtOrigin;
        maxDotProd = currentDotProd;
      }
    }
    
    //PRINT_INFO("ActiveCube %d's TopMarker is = %s\n", GetID().GetValue(),
    //           topMarker->GetCodeName());
    
    return *topMarker;
  }
  
  Radians Block::GetTopMarkerOrientation() const
  {
    Pose3d topMarkerPose;
    GetTopMarker(topMarkerPose);
    const Radians angle( topMarkerPose.GetRotation().GetAngleAroundZaxis() );
    return angle;
  }
  
  void Block::Visualize(const ColorRGBA& color) const
  {
    Pose3d vizPose = GetPose().GetWithRespectToRoot();
    _vizHandle = _vizManager->DrawCuboid(GetID().GetValue(), _size, vizPose, color);
  }
  
  void Block::EraseVisualization() const
  {
    // Erase the main object
    if(_vizHandle != VizManager::INVALID_HANDLE) {
      _vizManager->EraseVizObject(_vizHandle);
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
#       include "engine/BlockDefinitions.h"
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
  
  
  
  RotationAmbiguities const& Block_Cube1x1::GetRotationAmbiguities() const
  {
    static const RotationAmbiguities kFullyAmbiguous(true, {
        RotationMatrix3d({1,0,0,  0,1,0,  0,0,1}),
        RotationMatrix3d({0,1,0,  1,0,0,  0,0,1}),
        RotationMatrix3d({0,1,0,  0,0,1,  1,0,0}),
        RotationMatrix3d({0,0,1,  0,1,0,  1,0,0}),
        RotationMatrix3d({0,0,1,  1,0,0,  0,1,0}),
        RotationMatrix3d({1,0,0,  0,0,1,  0,1,0})
    });
    
    return kFullyAmbiguous;
  }
  
#pragma mark ---  Block_2x1 Implementation ---
  
  //const ObjectType Block_2x1::BlockType = Block::NumTypes++;
  
  
  
  RotationAmbiguities const& Block_2x1::GetRotationAmbiguities() const
  {
    // TODO: We don't have any 2x1 blocks anymore, but these ambiguities probably need revisiting
    static const RotationAmbiguities kRotationAmbiguities(true, {
      RotationMatrix3d({1,0,0,  0,1,0,  0,0,1}),
      RotationMatrix3d({1,0,0,  0,0,1,  0,1,0})
    });
    
    return kRotationAmbiguities;
  }



} // namespace Vector
} // namespace Anki
