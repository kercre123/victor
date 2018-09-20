/**
* File: behaviorRequestToGoHome.h
*
* Author: Matt Michini
* Created: 01/23/18
*
* Description: Find a face, turn toward it, and request to be taken back
*              to the charger.
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_Behaviors_BehaviorRequestToGoHome_H__
#define __Engine_Behaviors_BehaviorRequestToGoHome_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "coretech/common/engine/robotTimeStamp.h"

namespace Anki {
namespace Vector {
  
enum class AnimationTrigger : int32_t;
  
class BehaviorRequestToGoHome : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorRequestToGoHome(const Json::Value& config);
  
public:
  virtual ~BehaviorRequestToGoHome() override {}
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = false;
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
private:
  enum class EState {
    Init,
    FindingFaces,
    Requesting
  };
  
  enum class ERequestType {
    Normal,  // normal request
    Severe,  // severe request
    LowPower // time to transition to low-power mode
  };
  
  struct RequestParams {
    RequestParams();
    int numRequests;
    AnimationTrigger requestAnimTrigger;
    AnimationTrigger getoutAnimTrigger;
    AnimationTrigger waitLoopAnimTrigger;
    
    // How long to loop idle anims before transitioning to next request/stage
    float idleWaitTime_sec;
  };
  

  struct InstanceConfig {
    InstanceConfig();
    RequestParams normalRequest; // params to use for 'normal' requests
    RequestParams severeRequest; // params to use for 'severe' requests
    
    AnimationTrigger pickupAnimTrigger;
    
    // The maximum allowed 'age' for a face before resorting
    // to a search. i.e. if we have a known face but it's
    // older than this, search for faces anyway.
    float maxFaceAge_sec;

    // Sub-behavior used to search in place for a face
    ICozmoBehaviorPtr findFacesBehavior;
  };

  struct DynamicVariables {
    DynamicVariables(const InstanceConfig& iConfig);
    // pointer to the currently active request params
    const RequestParams* currRequestParamsPtr;
    ERequestType   currRequestType;
    EState         state;
    // Counters for number of requests that have happened
    int numNormalRequests;
    int numSevereRequests;
    // robot image timestamp at the time the behavior was activated
    RobotTimeStamp_t imageTimestampWhenActivated;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  void TransitionToCheckForFaces();
  void TransitionToSearchingForFaces();
  void TransitionToRequestAnim();
  void TransitionToRequestWaitLoopAnim();
  void TransitionToRequestGetoutAnim();
  void TransitionToLowPowerMode();
  
  void LoadConfig(const Json::Value& config);
  
  // Determine which request type should be active
  // and load the appropriate parameter set
  void UpdateCurrRequestTypeAndLoadParams();
};
  

} // namespace Vector
} // namespace Anki

#endif // __Engine_Behaviors_BehaviorRequestToGoHome_H__
