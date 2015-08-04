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

#ifndef COZMO_BEHAVIOR_LOOK_FOR_FACES_H
#define COZMO_BEHAVIOR_LOOK_FOR_FACES_H

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

#include "messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
  class BehaviorLookForFaces : public IBehavior
  {
  public:
    
    BehaviorLookForFaces(Robot& robot, const Json::Value& config);
    
    virtual Result Init() override;
    
    // Always runnable for now?
    virtual bool IsRunnable() const override { return true; }
    
    virtual Status Update(float currentTime_sec) override;
    
    virtual Result Interrupt() override;
    
    virtual const std::string& GetName() const override {
      static const std::string name("LookForFaces");
      return name;
    }
    
    virtual bool GetRewardBid(Reward& reward);
    
  private:
    
    void HandleRobotObservedObject(const ExternalInterface::RobotObservedObject& msg);
    void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg);
    
    void SetNextMovementTime();
    
    enum class State {
      LOOKING_AROUND,
      TRACKING_FACE,
      INTERRUPTED
    };
    
    State _currentState;
    
    TimeStamp_t _firstSeen_ms;
    TimeStamp_t _lastSeen_ms;
    
    f32 _trackingTimeout_sec;
    f32 _lastLookAround_sec;
    f32 _nextMovementTime_sec;
    
  }; // BehaviorLookForFaces
  
} // namespace Cozmo
} // namespace Anki

#endif // COZMO_BEHAVIOR_LOOK_FOR_FACES_H
