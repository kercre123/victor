/**
 * File: behaviorFindFaceAndThen.h
 *
 * Author: ross
 * Created: 2018-06-22
 *
 * Description: Finds a face either in the activation direction, or wherever one was last seen,
 *              and if it finds one, either delegates to a followup behavior or exits
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorFindFaceAndThen__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorFindFaceAndThen__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/smartFaceId.h"

namespace Anki {
namespace Cozmo {
  
class ISimpleFaceBehavior;

class BehaviorFindFaceAndThen : public ICozmoBehavior
{
public:
  // Returns the face found while running this behavior. It is reset upon activation, and persists after deactivation.
  SmartFaceID GetFoundFace() const { return _dVars.targetFace; }
  
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorFindFaceAndThen(const Json::Value& config);

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
    DriveOffCharger,
    LiftingHead,
    LookForFaceInMicDirection,
    TurnTowardsPreviousFace,
    FindFaceInCurrentDirection,
    SearchForFace,
    FollowupBehavior,
  };
  
  struct InstanceConfig {
    InstanceConfig();
    
    bool alwaysDetectFaces;
    
    float timeUntilCancelFaceLooking_s;
    float timeUntilCancelSearching_s;
    float timeUntilCancelFollowup_s;
    
    // if it starts on the charger, if can either leave the charger before looking for a face, or stay
    // on the charger looking for a face. If it leaves the charger, it will subsequently turn to the last seen face
    bool shouldLeaveChargerFirst;
    
    // if true, this behavior assumes it started facing in the dominant mic direction, unless it was on the charger.
    bool startedWithMicDirection;
    
    std::string searchBehaviorID;
    ICozmoBehaviorPtr searchFaceBehavior; // if set, this behavior will search for a face if one is not found
    
    std::string driveOffChargerBehaviorID;
    ICozmoBehaviorPtr driveOffChargerBehavior;
    
    std::string behaviorOnceFoundID;
    ICozmoBehaviorPtr behaviorOnceFound;
    bool behaviorOnceFoundIsSimpleFace;
    bool exitOnceFound;
  };

  struct DynamicVariables {
    DynamicVariables();
    State currentState;
    
    float stateEndTime_s;
    float stateMinTime_s;
    
    SmartFaceID targetFace;
    TimeStamp_t lastFaceTimeStamp_ms;
    
    TimeStamp_t activationTimeStamp_ms;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
  
  void TransitionToDrivingOffCharger();
  void TransitionToLookingInMicDirection();
  void TransitionToTurningTowardsFace();
  void TransitionToFindingFaceInCurrentDirection();
  void TransitionToSearchingForFace();
  void TransitionToFollowupBehavior();
  
  // If there is a face, and it is the most recent, and it shares the same origin, this returns true and sets the params
  bool GetRecentFaceSince( TimeStamp_t sinceTime_ms, SmartFaceID& faceID, TimeStamp_t& timeStamp_ms );
  bool GetRecentFace( SmartFaceID& faceID, TimeStamp_t& timeStamp_ms );
  
  void SetState_internal(State state, const std::string& stateName);

};


} // namespace Cozmo
} // namespace Anki


#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorFindFaceAndThen__
