/**
 * File: charger.h
 *
 * Author: Kevin Yoon
 * Date:   6/5/2015
 *
 * Description: Defines a Charger object, which is a type of DockableObject
 *
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_COZMO_BASESTATION_CHARGER_H
#define ANKI_COZMO_BASESTATION_CHARGER_H

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/quad.h"

#include "anki/vision/basestation/observableObject.h"

#include "anki/cozmo/basestation/comms/robot/robotMessages.h"

#include "anki/vision/MarkerCodeDefinitions.h"

#include "anki/cozmo/basestation/actionableObject.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

namespace Anki {
  
  namespace Cozmo {
    
    // Note that a ramp's origin (o) is the bottom right vertex of this diagram:
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
    
    class Charger : public ActionableObject
    {
    public:
      class Type : public ObjectType
      {
        Type(const std::string& name) : ObjectType(name) { }
      public:
        static const Type BASIC_CHARGER;
      };
      
      Charger();
      
      virtual ObjectType GetType() const override { return Type::BASIC_CHARGER; }
      virtual const Point3f& GetSize() const override { return _size; }
      
      f32     GetHeight() const { return Height; }
      Radians GetAngle()  const { return Angle;  }
      
      const Vision::KnownMarker* GetFrontMarker() const { return _frontMarker; }
      
      typedef enum : u8 {
        ASCENDING,
        DESCENDING,
        UNKNOWN
      } TraversalDirection;
      
      // Determine whether a robot will ascend or descend the ramp, based on its
      // relative pose. If it is above the ramp, it must be descending. If it
      // is on the same level as the ramp, it must be ascending. If it can't be
      // determined, UNKNOWN is returned.
      TraversalDirection WillAscendOrDescend(const Pose3d& robotPose) const;
      
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
      virtual ~Charger();
      
      virtual Charger*   CloneType() const override;
      virtual void    Visualize(const ColorRGBA& color) override;
      virtual void    EraseVisualization() override;
      

      virtual Point3f GetSameDistanceTolerance()  const override;
      
    protected:
      
      // Model dimensions in mm (perhaps these should come from a configuration
      // file instead?)
      constexpr static const f32 Width          = 85.f;
      constexpr static const f32 Height         = 20.f;
      constexpr static const f32 SlopeLength    = 50.f;
      constexpr static const f32 PlatformLength = 100.f;
      constexpr static const f32 TopMarkerSize  = 30.f;
      constexpr static const f32 FrontMarkerDistance = 20.f; // along sloped surface (at angle below)
      constexpr static const f32 PreAscentDistance  = 90.f; // for ascending from bottom
      constexpr static const f32 PreDescentDistance = 30.f; // for descending from top
      constexpr static const f32 Angle = 0.17f; // of first part of ramp, using vertex 18
      
      virtual const std::vector<Point3f>& GetCanonicalCorners() const override;
      
      Point3f _size;
      
      const Vision::KnownMarker* _frontMarker;
      const Vision::KnownMarker* _topMarker;
      
      Pose3d _preAscentPose;
      Pose3d _preDescentPose;
      
      VizManager::Handle_t _vizHandle;
      
      virtual bool IsPreActionPoseValid(const PreActionPose& preActionPose,
                                        const Pose3d* reachableFromPose,
                                        const std::vector<std::pair<Quad2f,ObjectID> >& obstacles) const override;
      
      
    }; // class Charger
    
    inline const Pose3d& Charger::GetPreAscentPose() const {
      return _preAscentPose;
    }
    
    inline const Pose3d& Charger::GetPreDescentPose() const {
      return _preDescentPose;
    }
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_CHARGER_H
