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
                        const float markerSize_mm)
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
        {0.f, Y_AXIS_3D()},  {M_PI_2, Y_AXIS_3D()},  {-M_PI_2, Y_AXIS_3D()},  {M_PI, Y_AXIS_3D()}
      }};
      
      // Add a pre-dock pose to each face, at fixed distance normal to the face,
      // and one for each orientation of the block
      {
        for(auto const& Rvec : preActionPoseRotations) {
          
          for (auto v : BLOCK_PREDOCK_POSE_OFFSETS) {
            Pose3d preDockPose(M_PI_2 + v.GetAngle().ToFloat(), Z_AXIS_3D(),  {{v.GetX() , -v.GetY(), -halfHeight}}, &marker->GetPose());
            preDockPose.RotateBy(Rvec);
            AddPreActionPose(PreActionPose::DOCKING, marker, preDockPose);
          }

        }
      }
      
      // Add a pre-placement pose to each face, where the robot will be sitting
      // relative to the face when we put down the block -- one for each
      // orientation of the block
      {
        const f32 DefaultPrePlacementDistance = ORIGIN_TO_LOW_LIFT_DIST_MM - DRIVE_CENTER_OFFSET;
        for(auto const& Rvec : preActionPoseRotations) {
          Pose3d prePlacementPose(M_PI_2, Z_AXIS_3D(),  {{0.f, -DefaultPrePlacementDistance, -halfHeight}}, &marker->GetPose());
          prePlacementPose.RotateBy(Rvec);
          AddPreActionPose(PreActionPose::PLACEMENT, marker, prePlacementPose);
        }
      }
      
      // Store a pointer to the marker on each face:
      markersByFace_[whichFace] = marker;
      
      //facesWithMarkerCode_[marker.GetCode()].push_back(whichFace);
      
    } // AddFace()
    
    Block::Block(const ObjectType type)
    : ActionableObject(ObjectFamily::Block, type)
    , _size(LookupBlockInfo(_type).size)
    , _name(LookupBlockInfo(_type).name)
    , _vizHandle(VizManager::INVALID_HANDLE)
    {
      SetColor(LookupBlockInfo(_type).color);
               
      markersByFace_.fill(NULL);
      
      for(auto face : LookupBlockInfo(_type).faces) {
        AddFace(face.whichFace, face.code, face.size);
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

#pragma mark ---  ActiveCube Implementation ---
    
    std::vector<RotationMatrix3d> const& ActiveCube::GetRotationAmbiguities() const
    {
      
      // TODO: Adjust if/when active blocks aren't fully ambiguous
      static const std::vector<RotationMatrix3d> RotationAmbiguities; /* = {
        RotationMatrix3d({1,0,0,  0,1,0,  0,0,1}),
        RotationMatrix3d({0,1,0,  1,0,0,  0,0,1}),
        RotationMatrix3d({0,1,0,  0,0,1,  1,0,0}),
        RotationMatrix3d({0,0,1,  0,1,0,  1,0,0}),
        RotationMatrix3d({0,0,1,  1,0,0,  0,1,0}),
        RotationMatrix3d({1,0,0,  0,0,1,  0,1,0})
      };
                                                                       */
      
      return RotationAmbiguities;
    }
    
    ActiveCube::ActiveCube(ObjectType type)
    : Block(type)
    , _activeID(-1)
    {
      // For now, assume 6 different markers, so we can avoid rotation ambiguities
      // Verify that here by making sure a set of markers has as many elements
      // as the original list:
      std::list<Vision::KnownMarker> const& markerList = GetMarkers();
      std::set<Vision::Marker::Code> uniqueCodes;
      for(auto & marker : markerList) {
        uniqueCodes.insert(marker.GetCode());
      }
      CORETECH_ASSERT(uniqueCodes.size() == markerList.size());
    }
  
    void ActiveCube::SetLEDs(const WhichBlockLEDs whichLEDs,
                             const ColorRGBA& onColor,
                             const ColorRGBA& offColor,
                             const u32 onPeriod_ms,
                             const u32 offPeriod_ms,
                             const u32 transitionOnPeriod_ms,
                             const u32 transitionOffPeriod_ms,
                             const bool turnOffUnspecifiedLEDs)
    {
      static const u8 FIRST_BIT = 0x01;
      u8 shiftedLEDs = static_cast<u8>(whichLEDs);
      for(u8 iLED=0; iLED<NUM_LEDS; ++iLED) {
        // If this LED is specified in whichLEDs (its bit is set), then
        // update
        if(shiftedLEDs & FIRST_BIT) {
          _ledState[iLED].onColor      = onColor;
          _ledState[iLED].offColor     = offColor;
          _ledState[iLED].onPeriod_ms  = onPeriod_ms;
          _ledState[iLED].offPeriod_ms = offPeriod_ms;
          _ledState[iLED].transitionOnPeriod_ms = transitionOnPeriod_ms;
          _ledState[iLED].transitionOffPeriod_ms = transitionOffPeriod_ms;
        } else if(turnOffUnspecifiedLEDs) {
          _ledState[iLED].onColor      = NamedColors::BLACK;
          _ledState[iLED].offColor     = NamedColors::BLACK;
          _ledState[iLED].onPeriod_ms  = 1000;
          _ledState[iLED].offPeriod_ms = 1000;
          _ledState[iLED].transitionOnPeriod_ms = 0;
          _ledState[iLED].transitionOffPeriod_ms = 0;
        }
        shiftedLEDs = shiftedLEDs >> 1;
      }
    }
    
    void ActiveCube::SetLEDs(const std::array<u32,NUM_LEDS>& onColors,
                             const std::array<u32,NUM_LEDS>& offColors,
                             const std::array<u32,NUM_LEDS>& onPeriods_ms,
                             const std::array<u32,NUM_LEDS>& offPeriods_ms,
                             const std::array<u32,NUM_LEDS>& transitionOnPeriods_ms,
                             const std::array<u32,NUM_LEDS>& transitionOffPeriods_ms)
    {
      for(u8 iLED=0; iLED<NUM_LEDS; ++iLED) {
        _ledState[iLED].onColor      = onColors[iLED];
        _ledState[iLED].offColor     = offColors[iLED];
        _ledState[iLED].onPeriod_ms  = onPeriods_ms[iLED];
        _ledState[iLED].offPeriod_ms = offPeriods_ms[iLED];

        // Handle some special cases (we want to avoid on/off times of 0 for the
        // sake of the real active blocks)
        if(onPeriods_ms[iLED] == 0 && offPeriods_ms[iLED] > 0) {
          // Looks like we mean for this LED to be solid "off" color
          _ledState[iLED].onColor = offColors[iLED];
          _ledState[iLED].onPeriod_ms = u32_MAX/2;
        }
        else if(offPeriods_ms[iLED] == 0 && onPeriods_ms[iLED] > 0) {
          // Looks like we mean for this LED to be solid "on" color
          _ledState[iLED].offColor = onColors[iLED];
          _ledState[iLED].offPeriod_ms = u32_MAX/2;
        }
        else if(onPeriods_ms[iLED]==0 && offPeriods_ms[iLED]==0) {
          // Looks like we mean for this LED to actually turn off
          _ledState[iLED].onColor = 0;
          _ledState[iLED].offColor = 0;
          _ledState[iLED].onPeriod_ms = u32_MAX/2;
          _ledState[iLED].offPeriod_ms = u32_MAX/2;
        }
        
        _ledState[iLED].transitionOnPeriod_ms = transitionOnPeriods_ms[iLED];
        _ledState[iLED].transitionOffPeriod_ms = transitionOffPeriods_ms[iLED];
      }
    }
    
    void ActiveCube::MakeStateRelativeToXY(const Point2f& xyPosition, MakeRelativeMode mode)
    {
      WhichBlockLEDs referenceLED = WhichBlockLEDs::NONE;
      switch(mode)
      {
        case RELATIVE_LED_MODE_OFF:
          // Nothing to do
          return;
          
        case RELATIVE_LED_MODE_BY_CORNER:
          referenceLED = GetCornerClosestToXY(xyPosition);
          break;
          
        case RELATIVE_LED_MODE_BY_SIDE:
          referenceLED = GetFaceClosestToXY(xyPosition);
          break;
          
        default:
          PRINT_NAMED_ERROR("ActiveCube.MakeStateRelativeToXY", "Unrecognized relateive LED mode %d.\n", mode);
          return;
      }
      
      switch(referenceLED)
      {
        //
        // When using upper left corner (of current top face) as reference corner:
        //
        //   OR
        //
        // When using upper side (of current top face) as reference side:
        // (Note this is the current "Left" face of the block.)
        //
          
        case WhichBlockLEDs::FRONT_RIGHT:
        case WhichBlockLEDs::FRONT:
          // Nothing to do
          return;
          
        case WhichBlockLEDs::FRONT_LEFT:
        case WhichBlockLEDs::LEFT:
          // Rotate clockwise one slot
          RotatePatternAroundTopFace(true);
          return;
          
        case WhichBlockLEDs::BACK_RIGHT:
        case WhichBlockLEDs::RIGHT:
          // Rotate counterclockwise one slot
          RotatePatternAroundTopFace(false);
          return;
          
        case WhichBlockLEDs::BACK_LEFT:
        case WhichBlockLEDs::BACK:
          // Rotate two slots (either direction)
          // TODO: Do this in one shot
          RotatePatternAroundTopFace(true);
          RotatePatternAroundTopFace(true);
          return;
          
        default:
          PRINT_STREAM_ERROR("ActiveCube.MakeStateRelativeToXY",
                            "Unexpected reference LED " << static_cast<int>(referenceLED) << ".");
          return;
      }
    } // MakeStateRelativeToXY()
    
    WhichBlockLEDs ActiveCube::MakeWhichLEDsRelativeToXY(const WhichBlockLEDs whichLEDs,
                                                         const Point2f& xyPosition,
                                                         MakeRelativeMode mode) const
    {
      WhichBlockLEDs referenceLED = WhichBlockLEDs::NONE;
      switch(mode)
      {
        case RELATIVE_LED_MODE_OFF:
          // Nothing to do
          return whichLEDs;
          
        case RELATIVE_LED_MODE_BY_CORNER:
          referenceLED = GetCornerClosestToXY(xyPosition);
          break;
          
        case RELATIVE_LED_MODE_BY_SIDE:
          referenceLED = GetFaceClosestToXY(xyPosition);
          break;
          
        default:
          PRINT_NAMED_ERROR("ActiveCube.MakeStateRelativeToXY", "Unrecognized relateive LED mode %d.\n", mode);
          return whichLEDs;
      }
      
      switch(referenceLED)
      {
        //
        // When using upper left corner (of current top face) as reference corner:
        //
        //  OR
        //
        // When using upper side (of current top face) as reference side:
        // (Note this is the current "Left" face of the block.)
        //
        
        case WhichBlockLEDs::FRONT_RIGHT:
        case WhichBlockLEDs::FRONT:
          // Nothing to do
          return whichLEDs;
          
        case WhichBlockLEDs::FRONT_LEFT:
        case WhichBlockLEDs::LEFT:
          // Rotate clockwise one slot
          return RotateWhichLEDsAroundTopFace(whichLEDs, true);
          
        case WhichBlockLEDs::BACK_RIGHT:
        case WhichBlockLEDs::RIGHT:
          // Rotate counterclockwise one slot
          return RotateWhichLEDsAroundTopFace(whichLEDs, false);

        case WhichBlockLEDs::BACK_LEFT:
        case WhichBlockLEDs::BACK:
          // Rotate two slots (either direction)
          // TODO: Do this in one shot
          return RotateWhichLEDsAroundTopFace(RotateWhichLEDsAroundTopFace(whichLEDs, true), true);
    
        default:
          PRINT_STREAM_ERROR("ActiveCube.MakeStateRelativeToXY",
                            "Unexpected reference LED " << static_cast<int>(referenceLED) << ".");
          return whichLEDs;
      }
    } // MakeWhichLEDsRelativeToXY()
    
    /*
    void ActiveCube::TurnOffAllLEDs()
    {
      SetLEDs(WhichBlockLEDs::ALL, NamedColors::BLACK, 0, 0);
    }
     */
    
    void ActiveCube::Identify()
    {
      // TODO: Actually get activeID from flashing LEDs instead of using a single hard-coded value
      switch(_markers.front().GetCode())
      {
        case Vision::MARKER_1:
        case Vision::MARKER_LIGHTNINGBOLT_01:
          _activeID = 0;
          break;
          
        case Vision::MARKER_INVERTED_1:
        case Vision::MARKER_LIGHTNINGBOLTHOLLOW_01:
          _activeID = 1;
          break;
          
        case Vision::MARKER_INVERTED_LIGHTNINGBOLT_01:
          _activeID = 2;
          break;

        case Vision::MARKER_INVERTED_LIGHTNINGBOLTHOLLOW_01:
          _activeID = 3;
          break;
          
        default:
          _activeID = -1;
          PRINT_NAMED_ERROR("ActiveCube.Identify.UnknownID",
                            "ActiveID not defined for block with front marker = %d\n",
                            _markers.front().GetCode());
      }
    } // Identify()
    
    std::map<s32,bool>& ActiveCube::GetAvailableIDs()
    {
      static std::map<s32,bool> availableIDs;
      return availableIDs;
    }
    
    void ActiveCube::RegisterAvailableID(s32 activeID)
    {
      if(ActiveCube::GetAvailableIDs().count(activeID) > 0) {
        PRINT_NAMED_WARNING("ActiveCube.RegisterAvailableID",
                            "Ignoring duplicate registration of available ID %d.\n", activeID);
      } else {
        ActiveCube::GetAvailableIDs()[activeID] = false;
      }
      
    }
    
    void ActiveCube::ClearAvailableIDs()
    {
      ActiveCube::GetAvailableIDs().clear();
    }
    
    const Vision::KnownMarker& ActiveCube::GetTopMarker(Pose3d& topMarkerPoseWrtOrigin) const
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
    
    Radians ActiveCube::GetTopMarkerOrientation() const
    {
      Pose3d topMarkerPose;
      GetTopMarker(topMarkerPose);
      const Radians angle( topMarkerPose.GetRotation().GetAngleAroundZaxis() );
      return angle;
    }
    
    WhichBlockLEDs ActiveCube::GetCornerClosestToXY(const Point2f& xyPosition) const
    {
      // Get a vector from center of marker in its current pose to given xyPosition
      Pose3d topMarkerPose;
      const Vision::KnownMarker& topMarker = GetTopMarker(topMarkerPose);
      const Vec2f topMarkerCenter(topMarkerPose.GetTranslation());
      Vec2f v(xyPosition);
      v -= topMarkerCenter;
      
      if (topMarker.GetCode() != GetMarker(FaceName::TOP_FACE).GetCode()) {
        PRINT_NAMED_WARNING("ActiveCube.GetCornerClosestToXY.IgnoringBecauseBlockOnSide", "");
        return WhichBlockLEDs::FRONT_LEFT;
      }
      
      PRINT_STREAM_INFO("ActiveCube.GetCornerClosestToXY", "ActiveCube " << GetID().GetValue() << "'s TopMarker is = " << Vision::MarkerTypeStrings[topMarker.GetCode()] << ", angle = " << std::setprecision(3) << topMarkerPose.GetRotation().GetAngleAroundZaxis().getDegrees() << "deg");
      
      Radians angle = std::atan2(v.y(), v.x());
      angle -= topMarkerPose.GetRotationAngle<'Z'>();
      //assert(angle >= -M_PI && angle <= M_PI); // No longer needed: Radians class handles this
      
      WhichBlockLEDs whichLEDs = WhichBlockLEDs::NONE;
      if(angle > 0.f) {
        if(angle < M_PI_2) {
          // Between 0 and 90 degrees: Upper Right Corner
          PRINT_STREAM_INFO("ActiveCube.GetCornerClosestToXY", "Angle = " << std::setprecision(3) << angle.getDegrees() <<  "deg, Closest corner to (" << xyPosition.x() << "," << xyPosition.y() << "): Back Left");
          whichLEDs = WhichBlockLEDs::BACK_LEFT;
        } else {
          // Between 90 and 180: Upper Left Corner
          //assert(angle<=M_PI);
          PRINT_STREAM_INFO("ActiveCube.GetCornerClosestToXY", "Angle = " << std::setprecision(3) << angle.getDegrees() <<  "deg, Closest corner to (" << xyPosition.x() << "," << xyPosition.y() << "): Front Left");
          whichLEDs = WhichBlockLEDs::FRONT_LEFT;
        }
      } else {
        //assert(angle >= -M_PI);
        if(angle > -M_PI_2) {
          // Between -90 and 0: Lower Right Corner
          PRINT_STREAM_INFO("ActiveCube.GetCornerClosestToXY", "Angle = " << std::setprecision(3) << angle.getDegrees() <<  "deg, Closest corner to (" << xyPosition.x() << "," << xyPosition.y() << "): Back Right");
          whichLEDs = WhichBlockLEDs::BACK_RIGHT;
        } else {
          // Between -90 and -180: Lower Left Corner
          //assert(angle >= -M_PI);
          PRINT_STREAM_INFO("ActiveCube.GetCornerClosestToXY", "Angle = " << std::setprecision(3) << angle.getDegrees() <<  "deg, Closest corner to (" << xyPosition.x() << "," << xyPosition.y() << "): Front Right");
          whichLEDs = WhichBlockLEDs::FRONT_RIGHT;
        }
      }
      
      return whichLEDs;
    } // GetCornerClosestToXY()
    
    WhichBlockLEDs ActiveCube::GetFaceClosestToXY(const Point2f& xyPosition) const
    {
      // Get a vector from center of marker in its current pose to given xyPosition
      Pose3d topMarkerPose;
      const Vision::KnownMarker& topMarker = GetTopMarker(topMarkerPose);
      const Vec3f topMarkerCenter(topMarkerPose.GetTranslation());
      const Vec2f v(xyPosition.x()-topMarkerCenter.x(), xyPosition.y()-topMarkerCenter.y());
      
      if (topMarker.GetCode() != GetMarker(FaceName::TOP_FACE).GetCode()) {
        PRINT_NAMED_WARNING("ActiveCube.GetFaceClosestToXY.IgnoringBecauseBlockOnSide", "");
        return WhichBlockLEDs::FRONT;
      }
      
      
      PRINT_STREAM_INFO("ActiveCube.GetFaceClosestToXY", "ActiveCube " << GetID().GetValue() << "'s TopMarker is = " << Vision::MarkerTypeStrings[topMarker.GetCode()] << ", angle = " << std::setprecision(3) << topMarkerPose.GetRotation().GetAngleAroundZaxis().getDegrees() << "deg");
      
      Radians angle = std::atan2(v.y(), v.x());
      angle -= topMarkerPose.GetRotationAngle<'Z'>();
      
      WhichBlockLEDs whichLEDs = WhichBlockLEDs::NONE;
      if(angle < M_PI_4 && angle >= -M_PI_4) {
        // Between -45 and 45 degrees: Right Face
        whichLEDs = WhichBlockLEDs::BACK;
        PRINT_STREAM_INFO("ActiveCube.GetFaceClosestToXY", "Angle = " << std::setprecision(3) << angle.getDegrees() <<  "deg, Closest face to (" << xyPosition.x() << "," << xyPosition.y() << "): Back");
      }
      else if(angle < 3*M_PI_4 && angle >= M_PI_4) {
        // Between 45 and 135 degrees: Back Face
        whichLEDs = WhichBlockLEDs::LEFT;
        PRINT_STREAM_INFO("ActiveCube.GetFaceClosestToXY", "Angle = " << std::setprecision(3) << angle.getDegrees() <<  "deg, Closest face to (" << xyPosition.x() << "," << xyPosition.y() << "): Left");
      }
      else if(angle < -M_PI_4 && angle >= -3*M_PI_4) {
        // Between -45 and -135: Front Face
        whichLEDs = WhichBlockLEDs::RIGHT;
        PRINT_STREAM_INFO("ActiveCube.GetFaceClosestToXY", "Angle = " << std::setprecision(3) << angle.getDegrees() <<  "deg, Closest face to (" << xyPosition.x() << "," << xyPosition.y() << "): Right");
      }
      else {
        // Between -135 && +135: Left Face
        assert(angle < -3*M_PI_4 || angle > 3*M_PI_4);
        whichLEDs = WhichBlockLEDs::FRONT;
        PRINT_STREAM_INFO("ActiveCube.GetFaceClosestToXY", "Angle = " << std::setprecision(3) << angle.getDegrees() <<  "deg, Closest face to (" << xyPosition.x() << "," << xyPosition.y() << "): Front");
      }
      
      return whichLEDs;
    } // GetFaceClosestToXY()
    
/*
    WhichBlockLEDs ActiveCube::RotatePatternAroundTopFace(WhichBlockLEDs pattern, bool clockwise)
    {
      static const u8 MASK = 0x88; // 0b10001000
      const u8 oldPattern = static_cast<u8>(pattern);
      if(clockwise) {
        return static_cast<WhichBlockLEDs>( ((oldPattern << 1) & ~MASK) | ((oldPattern & MASK) >> 3) );
      } else {
        return static_cast<WhichBlockLEDs>( ((oldPattern >> 1) & ~MASK) | ((oldPattern & MASK) << 3) );
      }
    }
  */
    
    inline const u8* GetRotationLUT(bool clockwise)
    {
      static const u8 cwRotatedPosition[ActiveCube::NUM_LEDS] = {
        3, 0, 1, 2
      };
      static const u8 ccwRotatedPosition[ActiveCube::NUM_LEDS] = {
        1, 2, 3, 0
      };
      
      // Choose the appropriate LUT
      const u8* rotatedPosition = (clockwise ? cwRotatedPosition : ccwRotatedPosition);
      
      return rotatedPosition;
    }
    
    void ActiveCube::RotatePatternAroundTopFace(bool clockwise)
    {
      const u8* rotatedPosition = GetRotationLUT(clockwise);
      
      // Create the new state array
      std::array<LEDstate,NUM_LEDS> newState;
      for(u8 iLED=0; iLED<NUM_LEDS; ++iLED) {
        newState[rotatedPosition[iLED]] = _ledState[iLED];
      }
      
      // Swap new state into place
      std::swap(newState, _ledState);
    } // RotatePatternAroundTopFace()
    
    
    WhichBlockLEDs ActiveCube::RotateWhichLEDsAroundTopFace(WhichBlockLEDs whichLEDs, bool clockwise)
    {
      const u8* rotatedPosition = GetRotationLUT(clockwise);
      
      u8 rotatedWhichLEDs = 0;
      u8 currentBit = 1;
      for(u8 iLED=0; iLED<NUM_LEDS; ++iLED) {
        // Set the corresponding rotated bit if the current bit is set
        rotatedWhichLEDs |= ((currentBit & (u8)whichLEDs)>0) << rotatedPosition[iLED];
        currentBit = currentBit << 1;
      }

      return (WhichBlockLEDs)rotatedWhichLEDs;
    }
    
/*
    void ActiveCube::FillMessage(MessageSetBlockLights& m) const
    {
      m.blockID = _activeID;
      
      assert(m.onPeriod_ms.size() == NUM_LEDS);
      assert(m.offPeriod_ms.size() == NUM_LEDS);
      assert(m.onColor.size() == NUM_LEDS);
      assert(m.offColor.size() == NUM_LEDS);
      
      for(u8 iLED=0; iLED<NUM_LEDS; ++iLED) {
        m.onColor[iLED] = _ledState[iLED].onColor;
        m.offColor[iLED] = _ledState[iLED].offColor;
        m.onPeriod_ms[iLED]  = _ledState[iLED].onPeriod_ms;
        m.offPeriod_ms[iLED] = _ledState[iLED].offPeriod_ms;
        m.transitionOnPeriod_ms[iLED]  = _ledState[iLED].transitionOnPeriod_ms;
        m.transitionOffPeriod_ms[iLED] = _ledState[iLED].transitionOffPeriod_ms;
      }
      
#     if SAVE_SET_BLOCK_LIGHTS_MESSAGES_FOR_DEBUG
      {
        static int saveCtr=0;
        Json::Value jsonMsg = m.CreateJson();
        std::ofstream jsonFile("SetBlockLights_" + std::to_string(saveCtr++) + ".json", std::ofstream::out);
        fprintf(stdout, "Writing SetBlockLights message to JSON file.\n");
        jsonFile << jsonMsg.toStyledString();
        jsonFile.close();
      }
#     endif 
      
    }
*/

  } // namespace Cozmo
} // namespace Anki