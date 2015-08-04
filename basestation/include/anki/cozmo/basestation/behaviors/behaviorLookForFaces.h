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
  
  class LookForFacesBehavior : public IBehavior
  {
  public:
    
    LookForFacesBehavior(Robot& robot, const Json::Value& config);
    
    virtual Result Init() override;
    
    // Always runnable for now?
    virtual bool IsRunnable() const override { return true; }
    
    virtual Status Update() override;
    
    virtual const std::string& GetName() const override {
      static const std::string name("LookForFaces");
      return name;
    }
    
  private:
    
    void HandleRobotObservedObject(const ExternalInterface::RobotObservedObject& msg);
    void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg);
    
    enum class State {
      LOOKING_AROUND,
      TRACKING_FACE
    };
    
    State _currentState;
    
    ObjectID _trackingID;
    
    f32 _lastSeen;
    f32 _trackingTimeout_sec;
    
  }; // LookForFacesBehavior
  
} // namespace Cozmo
} // namespace Anki

#endif // COZMO_BEHAVIOR_LOOK_FOR_FACES_H
