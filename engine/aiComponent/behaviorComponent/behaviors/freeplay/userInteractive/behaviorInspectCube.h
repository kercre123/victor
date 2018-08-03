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
class BlockWorldFilter;
struct TargetStatus;

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
    CubeSetDown,
    GetOutBored,
  }; 

  void SetState_internal(InspectCubeState state, const std::string& stateName);

  void UpdateBoredomTimeouts(const TargetStatus& targetStatus);
  
  // Behavior State Machine 
  void TransitionToSearching();
  void UpdateSearching(const TargetStatus& targetStatus);
  void TransitionToTracking();
  void UpdateTracking(const TargetStatus& targetStatus);
  void TransitionToKeepaway();
  void TransitionToGetOutTargetLost();
  void TransitionToGetOutBored();

  // Helper Methods
  float GetCurrentTimeInSeconds() const;

  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);
    std::string cubeSetDownBehaviorIDString;
    std::shared_ptr<ICozmoBehavior> cubeSetDownBehavior;
    std::shared_ptr<ICozmoBehavior> keepawayBehavior;
  };

  struct DynamicVariables {
    DynamicVariables(float startTime_s);
    InspectCubeState state; 
    float            behaviorStartTime_s;
    float            searchEndTime_s;
    float            trackingMinEndTime_s;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  float _currentTimeThisTick_s;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorInspectCube__
