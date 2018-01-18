/**
 * File: behaviorPeekABoo.h
 *
 * Author: Molly Jameson
 * Created: 2016-02-15
 *
 * Description: Behavior to do PeekABoo
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_Sparkable_BehaviorPeekABoo_H__
#define __Cozmo_Basestation_Behaviors_Sparkable_BehaviorPeekABoo_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/smartFaceId.h"

namespace Anki {
namespace Cozmo {

class BehaviorPeekABoo : public ICozmoBehavior
{
public:  
  // for COZMO-8914
  void PeekABooSparkStarted(float sparkTimeout);
  
protected:
  using super = ICozmoBehavior;
  
  enum State
  {
    DoingInitialReaction,
    TurningToFace,
    RequestPeekaBooAnim,
    WaitingToHideFace,
    WaitingToSeeFace,
    ReactingToPeekABooReturned,
    ReactingToNoUserInteraction,
    DoingFinalReaction
  };
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPeekABoo(const Json::Value& config);


  virtual bool WantsToBeActivatedBehavior() const override;

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;

  void TransitionToIntroAnim();
  void TransitionTurnToFace();
  void TransitionPlayPeekABooAnim();
  void TransitionWaitToHideFace();
  void TransitionWaitToSeeFace();
  void TransitionSeeFaceAfterHiding();
  void TransitionExit();
  void TransitionToNoUserInteraction();

  bool WasFaceHiddenAfterTimestamp(TimeStamp_t timestamp);
  
  AnimationTrigger GetPeekABooAnimation();
  SmartFaceID GetInteractionFace() const;

  IActionRunner* GetIdleAndReRequestAction(bool lockHead) const;
  
  void SetState_internal(State state, const std::string& stateName);
  
private:
  // Config numbers
  struct PeekABooParams{
    unsigned int minPeeks                          = 1;
    unsigned int maxPeeks                          = 3;
    unsigned int noUserInteractionTimeout_numIdles = 3;
    unsigned int numReRequestsPerTimeout           = 0;
    float        oldestFaceToConsider_MS           = 60000;
    bool         requireFaceConfirmBeforeRequest   = true;
    bool         playGetIn                         = true;
    float        minCoolDown_Sec                   = 0;
  };
  
  // ID of face we were just interacting with - used to give preference
  // to faces we were tracking before but should not be accessed directly
  mutable SmartFaceID _cachedFace;
  unsigned int   _numPeeksRemaining;
  unsigned int   _numPeeksTotal;
  float          _nextTimeWantsToBeActivated_Sec;
  float          _lastRequestTime_Sec;
  bool           _hasMadeFollowUpRequest;
  int            _turnToFaceRetryCount;
  bool           _stillSawFaceAfterRequest; // only a DAS tracking helper.
  State          _currentState;
  PeekABooParams _params;
  
  // maps a timestamp to the number of frames before that timestamp
  // during which eyes were not visible
  std::map<TimeStamp_t, int> _timestampEyeNotVisibleMap;

  // for COZMO-8914
  mutable float _timeSparkAboutToEnd_Sec;
  
  void UpdateTimestampSets();
  



};

}
}
 

#endif
