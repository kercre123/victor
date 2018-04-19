/**
 * File: BehaviorInspectCube.h
 *
 * Author: Sam Russell
 * Created: 2018-04-03
 *
 * Description: This behavior serves as a stage for implementing interactive, cube driven behavior transitions.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorInspectCube__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorInspectCube__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

// Forward Declarations
class BehaviorKeepaway;
class BlockWorldFilter;

class BehaviorInspectCube : public ICozmoBehavior
{
public: 
  virtual ~BehaviorInspectCube();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorInspectCube(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:
  enum class InspectCubeState{
    GetIn,
    Searching,
    Tracking,
    Keepaway,
    GetOutBored,
  }; 

  void SetState_internal(InspectCubeState state, const std::string& stateName);

  void UpdateBoredomTimeouts();
  void UpdateApproachState();
  
  // Behavior State Machine 
  void TransitionToSearching();
  void UpdateSearching();
  void TransitionToTracking();
  void UpdateTracking();
  void TransitionToKeepaway();
  void TransitionToGetOutBored();

  // Helper Methods
  float GetCurrentTimeInSeconds() const;

  // Target Tracking State and Methods - - - - -
  enum class TargetApproachState{
    Static,
    Retreating,
    Approaching
  };

  struct TargetStatus {
    TargetStatus();
    Pose3d  prevPose;
    float   lastValidTime_s;
    float   lastObservedTime_s;
    float   lastMovedTime_s;
    float   distance;
    float   prevDistance;
    bool    isValid = false;
    bool    isVisible = false;
    bool    isNotMoving = false;

  };

  bool CheckTargetStatus();
  bool CheckTargetObject();
  void UpdateTargetVisibility();
  void UpdateTargetAngle();
  void UpdateTargetDistance();
  void UpdateTargetMotion();
  bool TargetHasMoved(const ObservableObject* object);
  // Target Tracking State and Methods - - - - - 

  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);
    std::unique_ptr<BlockWorldFilter> targetFilter;
    std::shared_ptr<BehaviorKeepaway> keepawayBehavior;
    float   targetUnmovedGetOutTimeout_s;
    float   noValidTargetGetOutTimeout_s;
    float   targetHiddenGetOutTimeout_s;
    bool    useProxForDistance;
    float   triggerDistance_mm;
    float   minWithdrawalSpeed_mmpers;
    float   minWithdrawalDist_mm;
  };

  struct DynamicVariables {
    DynamicVariables();
    InspectCubeState    state; 
    ObjectID            targetID;
    TargetStatus        target;
    TargetApproachState approachState; 
    TargetApproachState prevApproachState; 
    float               behaviorStartTime_s;
    float               retreatStartDistance_mm;
    float               retreatStartTime_s;
    bool                nearApproachTriggered;
    bool                victorIsBored;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorInspectCube__
