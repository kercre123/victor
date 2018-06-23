/**
* File: behaviorDriveToFace.h
*
* Author: Kevin M. Karol
* Created: 2017-06-05
*
* Description: Behavior that turns towards and then drives to a face. If no face is known or found, it optionally tries to find it
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorDriveToFace_H__
#define __Cozmo_Basestation_Behaviors_BehaviorDriveToFace_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/smartFaceId.h"

namespace Anki {
namespace Cozmo {

class BehaviorDriveToFace : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorDriveToFace(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  
private:
  
  enum class State {
    Invalid=0,
    LookForFaceInMicDirection,
    TurnTowardsPreviousFace,
    FindFaceInCurrentDirection,
    SearchForFace,
    DriveToFace,
    AlreadyCloseEnough,
    TrackFace
  };
  
  struct InstanceConfig {
    InstanceConfig();
    
    bool alwaysDetectFaces;
    
    float timeUntilCancelFaceLooking_s;
    float timeUntilCancelSearching_s;
    float timeUntilCancelTracking_s;
    
    float minDriveToFaceDistance_mm;
    
    bool startedWithMicDirection; // if true, this behavior assumes it started facing in the dominant mic direction
    bool trackFaceOnceKnown; // if true, this behavior spends time tracking the face after driving up to it
    
    std::string searchBehaviorID;
    ICozmoBehaviorPtr searchFaceBehavior; // if set, this behavior will search for a face if one is not found
    
    AnimationTrigger animTooClose;
    AnimationTrigger animDriveOverride;
  };

  struct DynamicVariables {
    DynamicVariables();
    State currentState;
    
    float stateEndTime_s;
    
    

    SmartFaceID targetFace;
    TimeStamp_t lastFaceTimeStamp_ms;
    
    TimeStamp_t activationTimeStamp_ms;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
  
  void TransitionToLookingInMicDirection();
  void TransitionToTurningTowardsFace();
  void TransitionToFindingFaceInCurrentDirection();
  void TransitionToSearchingForFace();
  void TransitionToDrivingToFace();
  void TransitionToAlreadyCloseEnough();
  void TransitionToTrackingFace();
  
  // If there is a face, and it is the most recent, and it shares the same origin, this returns true and sets the params
  bool GetRecentFaceSince( TimeStamp_t sinceTime_ms, SmartFaceID& faceID, TimeStamp_t& timeStamp_ms );
  bool GetRecentFace( SmartFaceID& faceID, TimeStamp_t& timeStamp_ms );
  
  // Returns true if able to calculate distance to face - false otherwise
  bool CalculateDistanceToFace(Vision::FaceID_t faceID, float& distance);
  void SetState_internal(State state, const std::string& stateName);
  
  // Returns true if the robot is close enough to the face that he won't actually
  // drive forward when this behavior runs
  bool IsRobotAlreadyCloseEnoughToFace(Vision::FaceID_t faceID);

};


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_Behaviors_BehaviorDriveToFace_H__
