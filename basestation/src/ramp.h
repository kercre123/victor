/**
 * File: ramp.h
 *
 * Author: Andrew Stein
 * Date:   7/9/2014
 *
 * Description: Defines a Ramp object, which is a type of DockableObject
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_BASESTATION_RAMP_H
#define ANKI_COZMO_BASESTATION_RAMP_H

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/quad.h"

#include "anki/vision/basestation/observableObject.h"

#include "anki/cozmo/basestation/messages.h"

#include "anki/vision/MarkerCodeDefinitions.h"

#include "actionableObject.h"
#include "vizManager.h"

namespace Anki {
  
  namespace Cozmo {
    
    // Note that a ramp's origin (o) is the center of the "block" that makes up
    // its platform portion.
    //
    //   +------------+
    //   |              .
    //   |                .
    //   |     o            .
    //   |                    .
    //   |                      .
    //   *------------------------+
    //   <= Platform =><= Slope ==>
    //
    
    class Ramp : public ActionableObject
    {
    public:
      
      Ramp();
      
      static const ObjectType Type;
      
      virtual ObjectType GetType() const override { return Ramp::Type; }
      
      f32     GetHeight() const { return Height; }
      Radians GetAngle()  const { return Angle;  }
      
      const Vision::KnownMarker* GetFrontMarker() const { return _frontMarker; }
      const Vision::KnownMarker* GetTopMarker()   const { return _topMarker;   }
      
      // Return start poses (at Ramp's current position) for going up or down
      // the ramp. The distance for ascent is from the tip of the slope.  The
      // distance for descent is from the opposite edge of the ramp.
      const Pose3d& GetPreAscentPose()  const;
      const Pose3d& GetPreDescentPose() const;
      
      // Return final poses (at Ramp's current position) for a robot after it
      // has finished going up or down the ramp. Takes the robot's wheel base
      // as input since the assumption is that the robot will be level when its
      // back wheels have left the slope, meaning the robot's origin (between
      // its front two wheels) is wheel base away.
      Pose3d GetPostAscentPose(const float wheelBase)  const;
      Pose3d GetPostDescentPose(const float wheelBase) const;
      
      //
      // Inherited Virtual Methods
      //
      virtual ~Ramp();
      
      virtual Ramp*   CloneType() const override;
      virtual void    GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const override;
      virtual void    Visualize(const ColorRGBA& color) override;
      virtual void    EraseVisualization() override;
      virtual Quad2f  GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm = 0.f) const override;
      
      /*
      virtual void    GetPreDockPoses(const float distance_mm,
                                      std::vector<PoseMarkerPair_t>& poseMarkerPairs,
                                      const Vision::Marker::Code withCode = Vision::Marker::ANY_CODE) const override;
      */
      //virtual f32     GetDefaultPreDockDistance() const override;
      virtual Point3f GetSameDistanceTolerance()  const override;
      virtual Radians GetSameAngleTolerance()     const override;
      
      
      static ObjectType GetTypeByName(const std::string& name);

    protected:
      static const s32 NUM_CORNERS = 8;
      
      // Model dimensions in mm (perhaps these should come from a configuration
      // file instead?)
      constexpr static const f32 Width          = 62.f;
      constexpr static const f32 Height         = 44.f;
      constexpr static const f32 SlopeLength    = 110.f;
      constexpr static const f32 PlatformLength = 44.f;
      constexpr static const f32 MarkerSize     = 25.f;
      constexpr static const f32 FrontMarkerDistance = 50.f;
      constexpr static const f32 PreDockDistance    = 90.f; // for picking up from sides
      constexpr static const f32 PreAscentDistance  = 50.f; // for ascending from bottom
      constexpr static const f32 PreDescentDistance = 30.f; // for descending from top
      
      static const f32 Angle;
        
      static const std::array<Point3f, NUM_CORNERS> CanonicalCorners;
      
      const Vision::KnownMarker* _leftMarker;
      const Vision::KnownMarker* _rightMarker;
      const Vision::KnownMarker* _frontMarker;
      const Vision::KnownMarker* _topMarker;
      
      Pose3d _preAscentPose;
      Pose3d _preDescentPose;
      
      VizManager::Handle_t _vizHandle;
      //std::array<VizManager::Handle_t,3> _vizHandle;
      
    }; // class Ramp
    
    inline const Pose3d& Ramp::GetPreAscentPose() const {
      return _preAscentPose;
    }
    
    inline const Pose3d& Ramp::GetPreDescentPose() const {
      return _preDescentPose;
    }
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_RAMP_H
