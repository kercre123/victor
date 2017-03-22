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
  
protected:
  using super = IBehavior;
  
  enum DebugState
  {
    DoingInitialReaction,
    TurningToFace,
    RequestPeekaBooAnim,
    WaitingForHiddenFace,
    WaitingToSeeFace,
    ReactingToPeekABooReturned,
    DoingFinalReaction,
  };
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPeekABoo(Robot& robot, const Json::Value& config);


  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;

  virtual Result InitInternal(Robot& robot) override;
  
  virtual void   StopInternal(Robot& robot) override;

  void TransitionToIntroAnim(Robot& robot);
  void TransitionTurnToFace(Robot& robot);
  void TransitionReactToFace(Robot& robot);
  void TransitionWaitToHideFace(Robot& robot);
  void TransitionWaitToSeeFace(Robot& robot);
  void TransitionSeeFaceAfterHiding(Robot& robot);
  void TransitionExit(Robot& robot, bool facesFound = false);
  
  bool IsFaceHiddenNow(Robot& robot);
  
  void SelectFaceToTrack(const Robot& robot) const;
  // ID of face we are currently interested in
  mutable Vision::FaceID_t _targetFace = Vision::UnknownFaceID;
  TimeStamp_t  _timeSinceSeen_ms;
  unsigned int _numPeeksRemaining;
  unsigned int _numPeeksTotal;
  unsigned int _numShortPeeksRemaining;
  TimeStamp_t  _endTimeStamp;
  bool         _wantsShortReaction;
  float        _nextRunningTime;
  TimeStamp_t  _requestTimeStamp_Sec;
  bool         _stillSawFaceAfterRequest; // only a DAS tracking helper.
  
  // Config numbers
  unsigned int _minPeeks;
  unsigned int _maxPeeks;
  float        _percentCompleteSmallReaction;
  float        _percentCompleteMedReaction;
  float        _timeoutNoFacesTimeStamp_Sec;
  float        _waitIncrementTime_Sec;
  float        _oldestFaceToConsider_MS;
  float        _waitTimeFirst_Sec;
  bool         _requireFaceConfirmBeforeRequest;
  bool         _playGetIn;
  float        _minCoolDown_Sec;
  unsigned int _maxShortPeeks;

};

}
}
 

#endif
