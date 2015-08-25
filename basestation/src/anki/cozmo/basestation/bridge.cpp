/**
 * File: bridge.cpp
 *
 * Author: Andrew Stein
 * Date:   9/15/2014
 *
 * Description: Implements a Bridge object.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/common/types.h"

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

#include "anki/vision/MarkerCodeDefinitions.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "bridge.h"

namespace Anki {
  namespace Cozmo {
    
    static f32 GetLength(Bridge::Type type)
    {
      static const std::map<Bridge::Type, f32> Lengths = {
        {ObjectType::Bridge_LONG,  300.f},
        {ObjectType::Bridge_SHORT, 200.f}
      };
      
      auto iter = Lengths.find(type);
      if(iter == Lengths.end()) {
        PRINT_NAMED_ERROR("Bridge.GetLength.UnknownBridgeType",
                          "No length defined for bridge type %s (%d).\n",
                          ObjectTypeToString(type), type);
        return 0.f;
      } else {
        return iter->second;
      }
    } // GetLength()
    
    
    Bridge::Bridge(Type type)
    : MatPiece(type, {GetLength(type), 74.5f, 5.f})
    {
      Vision::MarkerType leftMarkerType = Vision::MARKER_0, rightMarkerType = Vision::MARKER_0, middleMarkerType = Vision::MARKER_0;
      f32 markerSize = 0.f;
      f32 length = 0.f;
      
      if(Type::Bridge_LONG == type) {
        length = 212.f;
        markerSize = 30.f;
        
//        leftMarkerType   = Vision::MARKER_BRIDGESUNLEFT;
//        rightMarkerType  = Vision::MARKER_BRIDGESUNRIGHT;
//        middleMarkerType = Vision::MARKER_BRIDGESUNMIDDLE;
      }
      else if(Type::Bridge_SHORT == type) {
        length = 112.f;
        markerSize = 30.f;
        
//        leftMarkerType   = Vision::MARKER_BRIDGEMOONLEFT;
//        rightMarkerType  = Vision::MARKER_BRIDGEMOONRIGHT;
//        middleMarkerType = Vision::MARKER_BRIDGEMOONMIDDLE;
      }
      else {
        PRINT_NAMED_ERROR("MatPiece.BridgeUnexpectedElse", "Should not get to else in if ladder constructing bridge-type mat.\n");
        return;
      }
      
      // Don't blindly call virtual GetSize() in the constructor because it may
      // not be the one we want. Explicitly ask for MatPiece's GetSize() implementation
      const Point3f& bridgeSize = MatPiece::GetSize();
      
      Pose3d preCrossingPoseLeft(0, Z_AXIS_3D(), {-bridgeSize.x()*.5f-30.f, 0.f, 0.f}, &GetPose());
      Pose3d preCrossingPoseRight(M_PI, Z_AXIS_3D(), {bridgeSize.x()*.5f+30.f, 0.f, 0.f}, &GetPose());
      
      //Pose3d leftMarkerPose(-M_PI_2, Z_AXIS_3D(), {-_size.x()*.5f+markerSize, 0.f, _size.z()});
      //leftMarkerPose *= Pose3d(-M_PI_2, X_AXIS_3D(), {0.f, 0.f, 0.f});
      Pose3d leftMarkerPose(-M_PI_2, X_AXIS_3D(), {-bridgeSize.x()*.5f+markerSize, 0.f, 0.f});
      
      //Pose3d rightMarkerPose(M_PI_2, Z_AXIS_3D(), { _size.x()*.5f-markerSize, 0.f, _size.z()});
      //rightMarkerPose *= Pose3d(-M_PI_2, X_AXIS_3D(), {0.f, 0.f, 0.f});
      Pose3d rightMarkerPose(-M_PI_2, X_AXIS_3D(), { bridgeSize.x()*.5f-markerSize, 0.f, 0.f});
      
      const Vision::KnownMarker* leftMarker  = &AddMarker(leftMarkerType,  leftMarkerPose,  markerSize);
      const Vision::KnownMarker* rightMarker = &AddMarker(rightMarkerType, rightMarkerPose, markerSize);
      AddMarker(middleMarkerType, Pose3d(-M_PI_2, X_AXIS_3D(), {0.f, 0.f, 0.f}), markerSize);
      
      CORETECH_ASSERT(leftMarker != nullptr);
      CORETECH_ASSERT(rightMarker != nullptr);
      
      if(preCrossingPoseLeft.GetWithRespectTo(leftMarker->GetPose(), preCrossingPoseLeft) == false) {
        PRINT_NAMED_ERROR("MatPiece.PreCrossingPoseLeftError", "Could not get preCrossingLeftPose w.r.t. left bridge marker.\n");
      }
      AddPreActionPose(PreActionPose::ENTRY, leftMarker, preCrossingPoseLeft);
      
      if(preCrossingPoseRight.GetWithRespectTo(rightMarker->GetPose(), preCrossingPoseRight) == false) {
        PRINT_NAMED_ERROR("MatPiece.PreCrossingPoseRightError", "Could not get preCrossingRightPose w.r.t. right bridge marker.\n");
      }
      AddPreActionPose(PreActionPose::ENTRY, rightMarker, preCrossingPoseRight);
      
    } // Bridge()
    
    
    void Bridge::GetCanonicalUnsafeRegions(const f32 padding_mm,
                                           std::vector<Quad3f>& regions) const
    {
      // Canonical unsafe regions for bridges run up the sides of the bridge
      regions = {{
        Quad3f({-0.5f*GetSize().x(), 0.5f*GetSize().y() + padding_mm, 0.f},
               {-0.5f*GetSize().x(), 0.5f*GetSize().y() - padding_mm, 0.f},
               { 0.5f*GetSize().x(), 0.5f*GetSize().y() + padding_mm, 0.f},
               { 0.5f*GetSize().x(), 0.5f*GetSize().y() - padding_mm, 0.f}),
        Quad3f({-0.5f*GetSize().x(),-0.5f*GetSize().y() + padding_mm, 0.f},
               {-0.5f*GetSize().x(),-0.5f*GetSize().y() - padding_mm, 0.f},
               { 0.5f*GetSize().x(),-0.5f*GetSize().y() + padding_mm, 0.f},
               { 0.5f*GetSize().x(),-0.5f*GetSize().y() - padding_mm, 0.f})
      }};
    }
    
    

    
  } // namespace Cozmo
} // namespace Anki