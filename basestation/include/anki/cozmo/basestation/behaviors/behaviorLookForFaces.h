/**
 * File: behaviorLookForFaces.h
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Defines Cozmo's "LookForFaces" behavior, which searches for people's
 *              faces and tracks/interacts with them if it finds one.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorLookForFaces_H__
#define __Cozmo_Basestation_Behaviors_BehaviorLookForFaces_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/proceduralFace.h"
#include "util/signals/simpleSignal_fwd.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
  class BehaviorLookForFaces : public IBehavior
  {
  public:
    
    BehaviorLookForFaces(Robot& robot, const Json::Value& config);
    
    virtual Result Init() override;
    
    // Always runnable for now?
    virtual bool IsRunnable(double currentTime_sec) const override { return true; }
    
    virtual Status Update(double currentTime_sec) override;
    
    virtual Result Interrupt(double currentTime_sec) override;
    
    virtual const std::string& GetName() const override {
      static const std::string name("LookForFaces");
      return name;
    }
    
    virtual bool GetRewardBid(Reward& reward);
    
  private:
    
    void HandleRobotObservedFace(const ExternalInterface::RobotObservedFace& msg);
    void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg);
    
    void UpdateBaselineFace(const Vision::TrackedFace* face);
    
    void SetNextMovementTime();
    
    enum class State {
      LOOKING_AROUND,
      MOVING,
      TRACKING_FACE,
      INTERRUPTED
    };
    
    State _currentState;
    
    TimeStamp_t _firstSeen_ms;
    TimeStamp_t _lastSeen_ms; // in frame (i.e. robot) time
    
    f32 _currentTime_sec;
    f32 _lastSeen_sec; // in engine time
    f32 _trackingTimeout_sec;
    f32 _lastLookAround_sec;
    f32 _nextMovementTime_sec;
    
    ProceduralFace _lastProceduralFace, _crntProceduralFace;;
    
    f32 _baselineEyeHeight;
    f32 _baselineIntraEyeDistance;
    f32 _baselineLeftEyebrowHeight;
    f32 _baselineRightEyebrowHeight;
    
    u32 _movementActionTag;
    u32 _animationActionTag;
    
    bool _isAnimating;
    
    std::vector<::Signal::SmartHandle> _eventHandles;
    
  }; // BehaviorLookForFaces
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorLookForFaces_H__
