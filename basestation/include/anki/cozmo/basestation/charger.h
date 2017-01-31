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

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/quad.h"

#include "anki/vision/basestation/observableObject.h"

#include "anki/vision/MarkerCodeDefinitions.h"

#include "anki/cozmo/basestation/actionableObject.h"
#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

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
    
    class Charger : public ActionableObject, public ActiveObject
    {
    public:
      
      Charger(ObjectType type = ObjectType::Charger_Basic);
      Charger(ActiveID activeID, FactoryID factoryID, ActiveObjectType activeObjectType);
      
      virtual const Point3f& GetSize() const override { return _size; }
      
      f32     GetHeight() const { return Height; }
      
      const Vision::KnownMarker* GetMarker() const { return _marker; }
      
      // Return pose of the robot when it's in the charger
      Pose3d GetRobotDockedPose()  const;
      // Return pose of charger wrt robot when the robot is on the charger
      static Pose3d GetDockPoseRelativeToRobot(const Robot& robot);
      
      // set pose to robot's pose and notify blockworld once done
      //void SetPoseRelativeToRobot(Robot& robot);
      
      
      //
      // Inherited Virtual Methods
      //
      virtual ~Charger();
      
      virtual Charger*   CloneType() const override;
      virtual void    Visualize(const ColorRGBA& color) const override;
      virtual void    EraseVisualization() const override;
      virtual bool CanIntersectWithRobot() const override { return true; }
      

      virtual Point3f GetSameDistanceTolerance()  const override;
      
      
      // Charger has no accelerometer so it should never be considered moving nor used for localization
      virtual bool IsMoving(TimeStamp_t* t = nullptr) const override { return false; }
      virtual void SetIsMoving(bool isMoving, TimeStamp_t t) override { }
      virtual bool CanBeUsedForLocalization() const override { return false; }
      
      
      constexpr static f32 GetLength() { return Length; }
      
    protected:
      
      // Model dimensions in mm (perhaps these should come from a configuration
      // file instead?)
      constexpr static const f32 WallWidth      = 10.f;
      constexpr static const f32 PlatformWidth  = 60.f;
      constexpr static const f32 Width          = 2*WallWidth + PlatformWidth;
      constexpr static const f32 Height         = 31.f;
      constexpr static const f32 SlopeLength    = 44.f;
      constexpr static const f32 PlatformLength = 42.f;
      constexpr static const f32 Length         = SlopeLength + PlatformLength + WallWidth;
      constexpr static const f32 MarkerHeight   = 20.f;
      constexpr static const f32 MarkerWidth    = 27.f;
      constexpr static const f32 MarkerZPosition   = 22.f;  // Middle of marker above ground
      constexpr static const f32 PreAscentDistance  = 100.f; // for ascending from bottom
      constexpr static const f32 RobotToChargerDistWhenDocked = 30.f;  // Distance from front of charger to robot origin when docked
      
      virtual const std::vector<Point3f>& GetCanonicalCorners() const override;
      
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
