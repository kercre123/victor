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

#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {
  
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
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override { modifiers.wantsToBeActivatedWhenOnCharger = false; }

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
private:
  
  void TransitionToSearchingForFaces();
  void TransitionToRequestAnim();
  void TransitionToRequestWaitLoopAnim();
  void TransitionToRequestGetoutAnim();
  void TransitionToLowPowerMode();
  
  void LoadConfig(const Json::Value& config);
  
  // Determine which request type should be active
  // and load the appropriate parameter set
  void UpdateCurrRequestTypeAndLoadParams();
  
  enum class EState {
    Init,
    FindingFaces,
    Requesting,
  } _state;
  
  enum class ERequestType {
    Normal,  // normal request
    Severe,  // severe request
    LowPower // time to transition to low-power mode
  } _currRequestType;
  
  struct RequestParams {
    int numRequests = 0;
    AnimationTrigger requestAnimTrigger  = AnimationTrigger::Count;
    AnimationTrigger getoutAnimTrigger   = AnimationTrigger::Count;
    AnimationTrigger waitLoopAnimTrigger = AnimationTrigger::Count;
    
    // How long to loop idle anims before transitioning to next request/stage
    float idleWaitTime_sec = 0.f;
  };
  
  struct {
    RequestParams normal; // params to use for 'normal' requests
    RequestParams severe; // params to use for 'severe' requests
    
    AnimationTrigger pickupAnimTrigger = AnimationTrigger::Count;
    
    // The maximum allowed 'age' for a face before resorting
    // to a search. i.e. if we have a known face but it's
    // older than this, search for faces anyway.
    float maxFaceAge_sec = 0.f;
  } _params;
  
  // pointer to the currently active request params
  RequestParams* _currRequestParams = &_params.normal;

  // Counters for number of requests that have happened
  int _numNormalRequests = 0;
  int _numSevereRequests = 0;
  
  // robot image timestamp at the time the behavior was activated
  TimeStamp_t _imageTimestampWhenActivated = 0;
  
  // Sub-behavior used to search in place for a face
  ICozmoBehaviorPtr _findFacesBehavior;

};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_Behaviors_BehaviorRequestToGoHome_H__
