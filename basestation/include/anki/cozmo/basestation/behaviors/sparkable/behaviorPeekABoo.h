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

#include "anki/cozmo/basestation/behaviors/iBehavior.h"

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
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPeekABoo(Robot& robot, const Json::Value& config);


  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;

  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

  void TransitionToIntroAnim(Robot& robot);
  void TransitionTurnToFace(Robot& robot);
  void TransitionPlayPeekABooAnim(Robot& robot);
  void TransitionWaitToHideFace(Robot& robot);
  void TransitionWaitToSeeFace(Robot& robot);
  void TransitionSeeFaceAfterHiding(Robot& robot);
  void TransitionExit(Robot& robot);
  void TransitionToNoUserInteraction(Robot& robot);

  bool WasFaceHiddenAfterTimestamp(Robot& robot, TimeStamp_t timestamp);
  
  AnimationTrigger GetPeekABooAnimation();
  Vision::FaceID_t GetInteractionFace(const Robot& robot) const;
  
  void SetState_internal(State state, const std::string& stateName);
  
private:
  // Config numbers
  struct PeekABooParams{
    unsigned int minPeeks                        = 1;
    unsigned int maxPeeks                        = 3;
    float        noUserInteractionTimeout_Sec    = 30.0f;
    float        oldestFaceToConsider_MS         = 60000;
    bool         requireFaceConfirmBeforeRequest = true;
    bool         playGetIn                       = true;
    float        minCoolDown_Sec                 = 0;
  };
  
  // ID of face we were just interacting with - used to give preference
  // to faces we were tracking before but should not be accessed directly
  mutable Vision::FaceID_t _cachedFace = Vision::UnknownFaceID;
  unsigned int   _numPeeksRemaining;
  unsigned int   _numPeeksTotal;
  float          _noUserInteractionTimeout_Sec;
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
  mutable float        _timeSparkAboutToEnd_Sec;
  
  void UpdateTimestampSets(Robot& robot);
  



};

}
}
 

#endif
