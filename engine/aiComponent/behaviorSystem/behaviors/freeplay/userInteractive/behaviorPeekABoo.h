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

#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorPeekABoo : public IBehavior
{
  
public:
  
  virtual bool CarryingObjectHandledInternally() const override { return false; }
  
  // for COZMO-8914
  void PeekABooSparkStarted(float sparkTimeout);
  
protected:
  using super = IBehavior;
  
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


  virtual bool IsRunnableInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  void TransitionToIntroAnim(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionTurnToFace(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionPlayPeekABooAnim(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionWaitToHideFace(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionWaitToSeeFace(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionSeeFaceAfterHiding(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionExit(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToNoUserInteraction(BehaviorExternalInterface& behaviorExternalInterface);

  bool WasFaceHiddenAfterTimestamp(BehaviorExternalInterface& behaviorExternalInterface, TimeStamp_t timestamp);
  
  AnimationTrigger GetPeekABooAnimation();
  Vision::FaceID_t GetInteractionFace(const BehaviorExternalInterface& behaviorExternalInterface) const;

  IActionRunner* GetIdleAndReRequestAction(BehaviorExternalInterface& behaviorExternalInterface, bool lockHead) const;
  
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
  mutable Vision::FaceID_t _cachedFace = Vision::UnknownFaceID;
  unsigned int   _numPeeksRemaining;
  unsigned int   _numPeeksTotal;
  float          _nextTimeIsRunnable_Sec;
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
  
  void UpdateTimestampSets(BehaviorExternalInterface& behaviorExternalInterface);
  



};

}
}
 

#endif
