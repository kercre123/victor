/**
 * File: behviorGameRequest.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-02-02
 *
 * Description: This is a base class for all behaviors which request mini game activities
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/behaviors/gameRequest/behaviorGameRequest.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"

namespace Anki {
namespace Cozmo {

#define DEBUG_BEHAVIOR_GAME_REQUEST_RUNNABLE 0

static const char* kMaxFaceAgeKey = "maxFaceAge_ms";

IBehaviorRequestGame::IBehaviorRequestGame(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
{
  if( config.isNull() ) {
    PRINT_NAMED_WARNING("IBehaviorRequestGame.Config.Error",
                        "Empty json config! This behavior will not function correctly");
  }
  else {
    {
      const Json::Value& val = config[kMaxFaceAgeKey];
      if( val.isUInt() ) {
        _maxFaceAge_ms = val.asUInt();
      }
    }
  }

  SubscribeToTags({{
    EngineToGameTag::RobotObservedFace,
    EngineToGameTag::RobotDeletedFace,
  }});

  SubscribeToTags({
    GameToEngineTag::DenyGameStart
  });
}

bool IBehaviorRequestGame::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  const bool hasFace = HasFace(robot);
  const bool ret = IsActing() || hasFace;

  if( DEBUG_BEHAVIOR_GAME_REQUEST_RUNNABLE ) {
    PRINT_NAMED_DEBUG("IBehaviorRequestGame.IsRunnable",
                      "'%s': %d: hasFace?%d (numBlocks=%d)",
                      GetName().c_str(),
                      ret,
                      hasFace,
                      GetNumBlocks(robot));
  }

  return ret;
}

Result IBehaviorRequestGame::InitInternal(Robot& robot, double currentTime_sec)
{
  _requestTime_s = -1.0f;
  
  return RequestGame_InitInternal(robot, currentTime_sec);
}

f32 IBehaviorRequestGame::GetRequestMinDelayComplete_s() const
{
  if( _requestTime_s < 0.0f ) {
    return -1.0f;
  }

  return _requestTime_s + _minRequestDelay_s;
}

void IBehaviorRequestGame::SendRequest(Robot& robot)
{
  using namespace ExternalInterface;

  robot.Broadcast( MessageEngineToGame( RequestGameStart() ) );
  _requestTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

void IBehaviorRequestGame::SendDeny(Robot& robot)
{
  using namespace ExternalInterface;

  robot.Broadcast( MessageEngineToGame( DenyGameStart() ) );
}

u32 IBehaviorRequestGame::GetNumBlocks(const Robot& robot) const
{  
  BlockWorldFilter filter;
  filter.OnlyConsiderLatestUpdate(false);
  filter.SetFilterFcn( [](ObservableObject* obj) {
    // Observable Objects includes "markerlessObject" cliffs.
    return obj->IsExistenceConfirmed() && obj->GetPoseState() == ObservableObject::PoseState::Known && obj->GetFamily() == ObjectFamily::LightCube;
    } );

  std::vector<ObservableObject*> blocks;
  robot.GetBlockWorld().FindMatchingObjects(filter, blocks);
  
  return Util::numeric_cast<u32>( blocks.size() );
}

ObservableObject* IBehaviorRequestGame::GetClosestBlock(const Robot& robot) const
{
  // TODO:(bn) re-ruse the filter?
  BlockWorldFilter filter;
  filter.OnlyConsiderLatestUpdate(false);
  filter.SetFilterFcn( [](ObservableObject* obj) {
      return obj->IsExistenceConfirmed() && obj->GetPoseState() == ObservableObject::PoseState::Known && obj->GetFamily() == ObjectFamily::LightCube;;
    } );

  return robot.GetBlockWorld().FindMostRecentlyObservedObject( filter );
}


ObjectID IBehaviorRequestGame::GetRobotsBlockID(const Robot& robot) const
{
  
  ObservableObject* closestObj = GetClosestBlock(robot);
  
  if( closestObj != nullptr ) {
    return closestObj->GetID();
  }
  else {
    // return a default object ID
    return {};
  }
}

bool IBehaviorRequestGame::GetLastBlockPose(Pose3d& pose) const
{
  if( _hasBlockPose ) {
    pose = _lastBlockPose;
  }
  return _hasBlockPose;
}

IBehavior::Status IBehaviorRequestGame::UpdateInternal(Robot& robot, double currentTime_sec)
{
  ObservableObject* obj = GetClosestBlock(robot);
  if( obj != nullptr ) {
    _hasBlockPose = true;
    _lastBlockPose = obj->GetPose();
  }
  
  return RequestGame_UpdateInternal(robot, currentTime_sec);
}


bool IBehaviorRequestGame::HasFace(const Robot& robot) const
{
  float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const u32 currTime_ms = Util::numeric_cast<u32>( std::floor( currentTime_sec * 0.001 ) );

  Pose3d waste;
  TimeStamp_t lastObservedFaceTime = robot.GetFaceWorld().GetLastObservedFace(waste);
  
  const bool hasFace = lastObservedFaceTime > 0 && lastObservedFaceTime + _maxFaceAge_ms > currTime_ms;

  return hasFace;
}

bool IBehaviorRequestGame::GetFacePose(const Robot& robot, Pose3d& facePose) const
{
  float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const u32 currTime_ms = Util::numeric_cast<u32>( std::floor( currentTime_sec * 0.001 ) );

  TimeStamp_t lastObservedFaceTime = robot.GetFaceWorld().GetLastObservedFace(facePose);
  
  const bool hasFace = lastObservedFaceTime > 0 && lastObservedFaceTime + _maxFaceAge_ms > currTime_ms;

  return hasFace;
}

void IBehaviorRequestGame::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedFace:
      HandleObservedFace(robot, event.GetData().Get_RobotObservedFace());
      break;
        
    case EngineToGameTag::RobotDeletedFace:
      HandleDeletedFace(event.GetData().Get_RobotDeletedFace());
      break;

    default:
      PRINT_NAMED_WARNING("IBehaviorRequestGame.InvalidTag",
                          "Received unexpected event with tag %hhu.", event.GetData().GetTag());
      break;
  }
}

void IBehaviorRequestGame::HandleWhileRunning(const GameToEngineEvent& event, Robot& robot)
{
  if( event.GetData().GetTag() == GameToEngineTag::DenyGameStart ) {
    HandleGameDeniedRequest(robot);
  }
  else {
    PRINT_NAMED_WARNING("IBehaviorRequestGame.InvalidTag",
                        "Received unexpected event with tag %hhu.", event.GetData().GetTag());
  }
}

void IBehaviorRequestGame::HandleObservedFace(const Robot& robot,
                                              const ExternalInterface::RobotObservedFace& msg)
{
  // If faceID not already set or we're not currently tracking the update the faceID
  if (_faceID == Face::UnknownFace) {
    _faceID = static_cast<Face::ID_t>(msg.faceID);
  }

  _lastFaceSeenTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}
  
void IBehaviorRequestGame::HandleDeletedFace(const ExternalInterface::RobotDeletedFace& msg)
{
  if (_faceID == msg.faceID) {
    _faceID = Face::UnknownFace;
  }
}

}
}
