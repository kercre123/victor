/**
 * File: behaviorLookForFaces.cpp
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Implements Cozmo's "LookForFaces" behavior, which searches for people's
 *              faces and tracks/interacts with them if it finds one.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/behaviors/behaviorLookForFaces.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoActions.h"

namespace Anki {
namespace Cozmo {

  LookForFacesBehavior::LookForFacesBehavior(Robot &robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _currentState(State::LOOKING_AROUND)
  {
    
  }
  
  Result LookForFacesBehavior::Init()
  {
    _currentState = State::LOOKING_AROUND;
    
    return RESULT_OK;
  }
  
  IBehavior::Status LookForFacesBehavior::Update()
  {
    switch(_currentState)
    {
      case State::LOOKING_AROUND:
        
        break;
        
      case State::TRACKING_FACE:
        
        break;
        
      default:
        
        return Status::FAILURE;
    } // switch(_currentState)
    
    return Status::RUNNING;
  } // Update()
  
  
#pragma mark -
#pragma mark Signal Handlers
  
  void LookForFacesBehavior::HandleRobotObservedObject(const ExternalInterface::RobotObservedObject &msg)
  {
    // we only care about observed faces
    if(msg.objectFamily == BlockWorld::ObjectFamily::HUMAN_HEADS)
    {
      
      switch(_currentState)
      {
          // If we aren't already tracking, start tracking the next face we see
        case State::LOOKING_AROUND:
          _trackingID = msg.objectID;
          _lastSeen = msg.
          _robot.EnableTrackToObject(_trackingID, false);
          break;

  }
  
  void LookForFacesBehavior::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction &msg)
  {
    if(msg.)
  }
  
} // namespace Cozmo
} // namespace Anki