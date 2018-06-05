/**
 * File: charger.h
 *
 * Author: Kevin Yoon
 * Date:   6/5/2015
 *
 * Description: Defines a Charger object, which is a type of ActionableObject
 *
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_COZMO_BASESTATION_CHARGER_H
#define ANKI_COZMO_BASESTATION_CHARGER_H

#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/math/quad.h"

#include "coretech/vision/engine/observableObject.h"

#include "coretech/vision/shared/MarkerCodeDefinitions.h"

#include "engine/actionableObject.h"
#include "engine/activeObject.h"
#include "engine/viz/vizManager.h"

namespace Anki {
  
  namespace Cozmo {
  
    class Robot;
    
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
      
      Charger(ObjectType type = ObjectType::Charger_Basic);
      
      virtual const Point3f& GetSize() const override { return _size; }
      
      f32     GetHeight() const { return kHeight; }
      
      const Vision::KnownMarker* GetMarker() const { return _marker; }
      
      // Return pose of the robot when it's in the charger
      Pose3d GetRobotDockedPose()  const;
      
      // Return the pose of the robot when it has just rolled off the
      // charger. It is roughly the first point at which the robot's
      // bounding quad no longer intersects the charger's.
      Pose3d GetRobotPostRollOffPose() const;
      
      // Return pose of charger wrt robot when the robot is on the charger
      static Pose3d GetDockPoseRelativeToRobot(const Robot& robot);
      
      // Returns a quad describing the area in front of the charger
      // that must be clear before the robot can dock with the charger.
      Quad2f GetDockingAreaQuad() const;
      
      //
      // Inherited Virtual Methods
      //
      virtual ~Charger();
      
      virtual Charger*   CloneType() const override;
      virtual void    Visualize(const ColorRGBA& color) const override;
      virtual void    EraseVisualization() const override;
      virtual bool CanIntersectWithRobot() const override { return true; }
      
      // Assume there is exactly one of these objects at a given time
      virtual bool IsUnique() const override  { return true; }

      virtual Point3f GetSameDistanceTolerance()  const override;
      
      
      // Charger has no accelerometer so it should never be considered moving nor used for localization
      virtual bool IsMoving(TimeStamp_t* t = nullptr) const override { return false; }
      virtual void SetIsMoving(bool isMoving, TimeStamp_t t) override { }
      virtual bool CanBeUsedForLocalization() const override { return false; }
      
      
      constexpr static f32 GetLength() { return kLength; }
      
    protected:
      
      // Model dimensions in mm (perhaps these should come from a configuration
      // file instead?)
      constexpr static const f32 kWallWidth      = 12.f;
      constexpr static const f32 kPlatformWidth  = 64.f;
      constexpr static const f32 kWidth          = 2*kWallWidth + kPlatformWidth;
      constexpr static const f32 kHeight         = 80.f;
      constexpr static const f32 kSlopeLength    = 94.f;
      constexpr static const f32 kPlatformLength = 0.f;
      constexpr static const f32 kLength         = kSlopeLength + kPlatformLength + kWallWidth;
      constexpr static const f32 kMarkerHeight   = 46.f;
      constexpr static const f32 kMarkerWidth    = 46.f;
      constexpr static const f32 kMarkerZPosition   = 48.5f;  // Middle of marker above ground
      constexpr static const f32 kPreAscentDistance  = 100.f; // for ascending from bottom
      constexpr static const f32 kRobotToChargerDistWhenDocked = 30.f;  // Distance from front of charger to robot origin when docked
      constexpr static const f32 kRobotToChargerDistPostRollOff = 80.f;  // Distance from front of charger to robot origin after just having rolled off the charger
      
      virtual const std::vector<Point3f>& GetCanonicalCorners() const override;
      
      virtual void GeneratePreActionPoses(const PreActionPose::ActionType type,
                                          std::vector<PreActionPose>& preActionPoses) const override;
      
      Point3f _size;
      
      const Vision::KnownMarker* _marker;
      
      mutable VizManager::Handle_t _vizHandle;
      
      virtual bool IsPreActionPoseValid(const PreActionPose& preActionPose,
                                        const Pose3d* reachableFromPose,
                                        const std::vector<std::pair<Quad2f,ObjectID> >& obstacles) const override;
      
      
    }; // class Charger
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_CHARGER_H
