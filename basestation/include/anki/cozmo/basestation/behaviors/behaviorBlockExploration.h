/**
 * File: behaviorBlockExploration.h
 *
 * Author: Trevor Dasch
 * Created: 1/27/16
 *
 * Description: Behavior where Cozmo attempt to find a block
 *              and return it to his starting position
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorBlockExploration_H__
#define __Cozmo_Basestation_Behaviors_BehaviorBlockExploration_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectIDs.h"
#include "anki/common/basestation/math/pose.h"

#include <vector>
#include <set>

namespace Anki {
  namespace Cozmo {
    
    // Forward declaration
    template<typename TYPE> class AnkiEvent;
    namespace ExternalInterface { class MessageEngineToGame; }
    
    class BehaviorBlockExploration : public IBehavior
    {
    protected:
      
      // Enforce creation through BehaviorFactory
      friend class BehaviorFactory;
      BehaviorBlockExploration(Robot& robot, const Json::Value& config);
      
    public:
      
      virtual ~BehaviorBlockExploration() override;
      
      virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override;
      
      void SetBlockExplorationHeadAngle(float angle_rads) { _lookAroundHeadAngle_rads = angle_rads; }
      
    protected:
      
      virtual Result InitInternal(Robot& robot, double currentTime_sec, bool isResuming) override;
      virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
      virtual Result InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt) override;
      virtual void   StopInternal(Robot& robot, double currentTime_sec) override;
      
      virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
      
    private:
      enum class State {
        Inactive,
        StartDriving,
        DrivingAround,
        StartLooking,
        LookingForObject,
        PickUpObject,
        PickingUpObject,
        ReturnObjectToCenter,
        ReturningObjectToCenter
      };
      
      enum class Destination {
        North = 0,
        West,
        South,
        East,
        Center,
        
        Count
      };
      
      // How long to wait before we trying to look around again (among other factors)
      constexpr static f32 kBlockExplorationCooldownDuration = 7;
      // How fast to rotate when looking around
      constexpr static f32 kDegreesRotatePerSec = 25;

      // How far to rotate when looking around
      constexpr static f32 kDegreesToRotate = 63;
      // The default radius (in mm) we assume exists for us to move around in
      constexpr static f32 kDefaultSafeRadius = 150;
      // Number of destinations we want to reach before resting for a bit (needs to be at least 2)
      constexpr static u32 kDestinationsToReach = 6;
      
      State _currentState = State::Inactive;
      Destination _currentDestination = Destination::North;
      f32 _lastBlockExplorationTime = 0;
      Pose3d _moveAreaCenter;
      f32 _safeRadius = kDefaultSafeRadius;
      u32 _currentDriveActionID = 0;
      u32 _currentLookActionID = 0;
      u32 _currentPickupObjectActionID = 0;
      u32 _currentPlaceObjectActionID = 0;
      u32 _numDestinationsLeft = kDestinationsToReach;
      f32 _lookAroundHeadAngle_rads = 0;
      
      std::set<ObjectID> _knownObjects;
      std::set<ObjectID> _seenObjects;
      std::set<uint32_t> _actionsInProgress;
      ObjectID _selectedObjectID;
      
      bool UnknownBlocksExist(const Robot& robot) const;
      Result StartMoving(Robot& robot);
      Pose3d GetDestinationPose(Destination destination);
      void ResetBehavior(Robot& robot, float currentTime_sec);
      Destination GetNextDestination(Destination current);
      void UpdateSafeRegion(const Vec3f& objectPosition);
      void ResetSafeRegion(Robot& robot);
      void ClearQueuedActions(Robot& robot);
      
      void HandleObjectObserved(const EngineToGameEvent& event, Robot& robot);
      void HandleCompletedAction(const EngineToGameEvent& event);
      void HandleRobotPutDown(const EngineToGameEvent& event, Robot& robot);
    };
    
    
  } // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorBlockExploration_H__
