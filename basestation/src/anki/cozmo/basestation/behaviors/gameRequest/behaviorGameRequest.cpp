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
#include "anki/cozmo/basestation/behaviors/gameRequest/behaviorGameRequest.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorTypesHelpers.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"

namespace Anki {
namespace Cozmo {

#define DEBUG_BEHAVIOR_GAME_REQUEST_RUNNABLE 0

static const char* kMaxFaceAgeKey = "maxFaceAge_ms";
static const char* kRequiredGameFlagsKey = "requiredGameFlags";

IBehaviorRequestGame::IBehaviorRequestGame(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _blockworldFilter( new BlockWorldFilter )
{
  if( config.isNull() ) {
    PRINT_NAMED_ERROR("IBehaviorRequestGame.Config.Error",
                        "Empty json config! This behavior will not function correctly");
  }
  else {
    {
      const Json::Value& val = config[kMaxFaceAgeKey];
      if( val.isUInt() ) {
        _maxFaceAge_ms = val.asUInt();

        PRINT_NAMED_DEBUG("IBehaviorRequestGame.MaxFaceAgeOverride",
                          "custom max face age value %dms",
                          _maxFaceAge_ms);
      }
    }
  }

  _blockworldFilter->OnlyConsiderLatestUpdate(false);
  _blockworldFilter->SetFilterFcn( std::bind( &IBehaviorRequestGame::FilterBlocks,
                                                    this,
                                                    &robot,
                                                    std::placeholders::_1) );
  
  
  // read required game flag. It must be specified
  const Json::Value& requiredGameFlagsJson = config[kRequiredGameFlagsKey];
  if ( !requiredGameFlagsJson.isNull())
  {
    _requiredGameFlags = BehaviorGameFlagFromString( requiredGameFlagsJson.asString() );
  }
  else
  {
    JsonTools::PrintJsonError(config, "IBehaviorRequestGame.NoGameFlag");
    DEV_ASSERT(!requiredGameFlagsJson.isNull(), "IBehaviorRequestGame.NoGameFlag");
  }
  
  SubscribeToTags({{
    EngineToGameTag::RobotObservedFace,
    EngineToGameTag::RobotDeletedFace
  }});

  SubscribeToTags({
    GameToEngineTag::DenyGameStart
  });
}

bool IBehaviorRequestGame::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  const Robot& robot = preReqData.GetRobot();
  
  const bool isGameAvailable = robot.GetBehaviorManager().IsAnyGameFlagAvailable(_requiredGameFlags);
  if ( !isGameAvailable )
  {
    if ( DEBUG_BEHAVIOR_GAME_REQUEST_RUNNABLE ) {
      PRINT_NAMED_DEBUG("IBehaviorRequestGame.IsRunnable", "'%s': GameFlag not available (%s). Behavior not runnable",
                        GetName().c_str(), BehaviorGameFlagToString(_requiredGameFlags) );
    }
    return false;
  }

  const bool hasFace = HasFace(robot);

  if( DEBUG_BEHAVIOR_GAME_REQUEST_RUNNABLE ) {
    PRINT_NAMED_DEBUG("IBehaviorRequestGame.IsRunnable",
                      "'%s': hasFace?%d (numBlocks=%d)",
                      GetName().c_str(),
                      hasFace,
                      GetNumBlocks(robot));
  }

  return hasFace;
}

Result IBehaviorRequestGame::InitInternal(Robot& robot)
{
  _requestTime_s = -1.0f;
  _robotsBlockID.UnSet();
  _badBlocks.clear();
  
  return RequestGame_InitInternal(robot);
}

void IBehaviorRequestGame::SendRequest(Robot& robot, bool initialRequest)
{
  using namespace ExternalInterface;

  robot.Broadcast( MessageEngineToGame( RequestGameStart(initialRequest) ) );
  _requestTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

void IBehaviorRequestGame::SendDeny(Robot& robot)
{
  using namespace ExternalInterface;

  robot.Broadcast( MessageEngineToGame( DenyGameStart() ) );
}

bool IBehaviorRequestGame::FilterBlocks( const Robot* robotPtr, const ObservableObject* obj) const
{

  // if we have the "cube roll" behavior unlocked, then only do a game request with an upright
  // cube. Otherwise, we don't care about the direction
  const bool forFreeplay = true;
  const bool isRollingUnlocked = robotPtr->GetProgressionUnlockComponent().IsUnlocked(UnlockId::RollCube,
                                                                                      forFreeplay);
  const bool upAxisOk = ! isRollingUnlocked ||
    obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS;

  // check to make sure we haven't failed to interact with the block recently
  const auto& whiteboard = robotPtr->GetAIComponent().GetWhiteboard();
  const bool recentlyFailed = whiteboard.DidFailToUse(obj->GetID(),
                                                      {{AIWhiteboard::ObjectUseAction::PickUpObject,
                                                        AIWhiteboard::ObjectUseAction::RollOrPopAWheelie}},
                                                      DefaultFailToUseParams::kTimeObjectInvalidAfterFailure_sec);
  
  // TODO:(bn) lee suggested we use != UNKNOWN instead of == known, so that we will still attempt to interact
  // with dirty blocks. I think the best would be to prefer Known, but fall back to Dirty as well
  
  return upAxisOk &&
    obj->IsPoseStateKnown() &&
    obj->GetFamily() == ObjectFamily::LightCube &&
    !recentlyFailed &&
    robotPtr->CanPickUpObject(*obj);

}

u32 IBehaviorRequestGame::GetNumBlocks(const Robot& robot) const
{
  std::vector<const ObservableObject*> blocks;
  robot.GetBlockWorld().FindLocatedMatchingObjects(*_blockworldFilter, blocks);
  
  return Util::numeric_cast<u32>( blocks.size() );
}

const ObservableObject* IBehaviorRequestGame::GetClosestBlock(const Robot& robot) const
{
  return robot.GetBlockWorld().FindMostRecentlyObservedObject( *_blockworldFilter );
}


ObjectID IBehaviorRequestGame::GetRobotsBlockID(const Robot& robot)
{
  if( ! _robotsBlockID.IsSet() ) {
    // set the block ID, but then leave it the same for the duration of the behavior
    const ObservableObject* closestObj = GetClosestBlock(robot);
  
    if( closestObj != nullptr ) {
      PRINT_NAMED_DEBUG("BehaviorRequestGame.SetRobotBlockID", "%d",
                        closestObj->GetID().GetValue());
      _robotsBlockID = closestObj->GetID();
    }
  }

  // either return the valid id, or an invalid ID if we don't have a block
  return _robotsBlockID;
}

bool IBehaviorRequestGame::SwitchRobotsBlock(const Robot& robot)
{
  if( ! _robotsBlockID.IsSet() ) {
    // robot doesn't have a block, so just try to get it now
    return GetRobotsBlockID(robot).IsSet();
  }

  // otherwise, try to find a new block that doesn't match this ID
  _badBlocks.insert(_robotsBlockID);

  // In this case (and only this case) we want to filter out _badBlocks, so make a new filter
  BlockWorldFilter filter( *_blockworldFilter );
  filter.SetFilterFcn( [this,&robot](const ObservableObject* obj) {
      return FilterBlocks(&robot, obj) && _badBlocks.find( obj->GetID() ) == _badBlocks.end();
    } );

  const ObservableObject* newBlock = robot.GetBlockWorld().FindMostRecentlyObservedObject( filter );
  if( newBlock != nullptr ) {
    PRINT_NAMED_DEBUG("BehaviorRequestGame.SwitchRobotsBlock", "switch from %d to %d",
                      _robotsBlockID.GetValue(),
                      newBlock->GetID().GetValue());

    _robotsBlockID = newBlock->GetID();
    return true;
  }

  return false;
}

bool IBehaviorRequestGame::GetLastBlockPose(Pose3d& pose) const
{
  if( _hasBlockPose ) {
    pose = _lastBlockPose;
  }
  return _hasBlockPose;
}

IBehavior::Status IBehaviorRequestGame::UpdateInternal(Robot& robot)
{
  const ObservableObject* obj = GetClosestBlock(robot);
  if( obj != nullptr ) {
    _hasBlockPose = true;
    _lastBlockPose = obj->GetPose();
  }
  
  return RequestGame_UpdateInternal(robot);
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

    case EngineToGameTag::CliffEvent:
      // handled in WhileRunning
      break;

    default:
      PRINT_NAMED_WARNING("IBehaviorRequestGame.InvalidTag",
                          "Received unexpected event with tag %hhu.", event.GetData().GetTag());
      break;
  }
}

void IBehaviorRequestGame::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  if( event.GetData().GetTag() == EngineToGameTag::CliffEvent ) {
    HandleCliffEvent(robot, event);
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
  if (_faceID == Vision::UnknownFaceID
      || _faceID != msg.faceID) {
    _faceID = static_cast<FaceID_t>(msg.faceID);
  }

  _lastFaceSeenTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}
  
void IBehaviorRequestGame::HandleDeletedFace(const ExternalInterface::RobotDeletedFace& msg)
{
  if (_faceID == msg.faceID) {
    _faceID = Vision::UnknownFaceID;
  }
}

}
}
