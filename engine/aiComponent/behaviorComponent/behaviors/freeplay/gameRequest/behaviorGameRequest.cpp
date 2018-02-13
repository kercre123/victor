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
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/gameRequest/behaviorGameRequest.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/actionInterface.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/dockingComponent.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/faceWorld.h"
#include "engine/voiceCommands/voiceCommandComponent.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"

namespace Anki {
namespace Cozmo {

#define DEBUG_BEHAVIOR_GAME_REQUEST_ACTIVATABLE 0

static const char* kMaxFaceAgeKey = "maxFaceAge_ms";


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorRequestGame::ICozmoBehaviorRequestGame(const Json::Value& config)
: ICozmoBehavior(config)
, _blockworldFilter( new BlockWorldFilter )
{
  if( config.isNull() ) {
    PRINT_NAMED_ERROR("ICozmoBehaviorRequestGame.Config.Error",
                        "Empty json config! This behavior will not function correctly");
  }
  else {
    {
      const Json::Value& val = config[kMaxFaceAgeKey];
      if( val.isUInt() ) {
        _maxFaceAge_ms = val.asUInt();

        PRINT_NAMED_DEBUG("ICozmoBehaviorRequestGame.MaxFaceAgeOverride",
                          "custom max face age value %dms",
                          _maxFaceAge_ms);
      }
    }
  }
  
  SubscribeToTags({{
    EngineToGameTag::RobotObservedFace,
    EngineToGameTag::RobotDeletedFace
  }});

  SubscribeToTags({{
    GameToEngineTag::DenyGameStart,
    GameToEngineTag::CanCozmoRequestGame
  }});
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehaviorRequestGame::InitBehavior()
{  
  _blockworldFilter->OnlyConsiderLatestUpdate(false);
  _blockworldFilter->SetFilterFcn( std::bind( &ICozmoBehaviorRequestGame::FilterBlocks,
                                             this,
                                             &GetBEI().GetProgressionUnlockComponent(),
                                             &GetBEI().GetAIComponent(),
                                             &GetBEI().GetRobotInfo().GetDockingComponent(),
                                             std::placeholders::_1) );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehaviorRequestGame::WantsToBeActivatedBehavior() const
{ 
  const bool hasFace = HasFace();

  if( DEBUG_BEHAVIOR_GAME_REQUEST_ACTIVATABLE ) {
    PRINT_NAMED_DEBUG("ICozmoBehaviorRequestGame.WantsToBeActivated",
                      "'%s': hasFace?%d (numBlocks=%d)",
                      GetDebugLabel().c_str(),
                      hasFace,
                      GetNumBlocks());
  }

  return hasFace;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehaviorRequestGame::OnBehaviorActivated()
{
  _requestTime_s = -1.0f;
  _robotsBlockID.UnSet();
  _badBlocks.clear();
  
  RequestGame_OnBehaviorActivated();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehaviorRequestGame::SendRequest()
{
  using namespace ExternalInterface;
  const auto& robotInfo = GetBEI().GetRobotInfo();
  robotInfo.GetContext()->GetVoiceCommandComponent()->ForceListenContext(VoiceCommand::VoiceCommandListenContext::SimplePrompt);

  //robot.Broadcast( MessageEngineToGame( RequestGameStart(GetRequiredUnlockID())) );
  _requestTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    
  GetBEI().GetRobotPublicStateBroadcaster().UpdateRequestingGame(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehaviorRequestGame::SendDeny()
{
  using namespace ExternalInterface;
  //robot.Broadcast( MessageEngineToGame( DenyGameStart() ) );
  GetBEI().GetRobotPublicStateBroadcaster().UpdateRequestingGame(false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehaviorRequestGame::FilterBlocks(ProgressionUnlockComponent* unlockComp, AIComponent* aiComp,
                                             DockingComponent* dockingComp, const ObservableObject* obj) const
{
  // if we have the "cube roll" behavior unlocked, then only do a game request with an upright
  // cube. Otherwise, we don't care about the direction
  const bool forFreeplay = true;
  const bool isRollingUnlocked = unlockComp->IsUnlocked(UnlockId::RollCube, forFreeplay);
  const bool upAxisOk = ! isRollingUnlocked ||
    obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS;

  // check to make sure we haven't failed to interact with the block recently
  const auto& whiteboard = aiComp->GetWhiteboard();
  const bool recentlyFailed = whiteboard.DidFailToUse(obj->GetID(),
                                                      {{AIWhiteboard::ObjectActionFailure::PickUpObject,
                                                        AIWhiteboard::ObjectActionFailure::RollOrPopAWheelie}},
                                                      DefaultFailToUseParams::kTimeObjectInvalidAfterFailure_sec);
  
  // TODO:(bn) lee suggested we use != UNKNOWN instead of == known, so that we will still attempt to interact
  // with dirty blocks. I think the best would be to prefer Known, but fall back to Dirty as well
  
  return upAxisOk &&
    obj->IsPoseStateKnown() &&
    obj->GetFamily() == ObjectFamily::LightCube &&
    !recentlyFailed &&
    dockingComp->CanPickUpObject(*obj);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u32 ICozmoBehaviorRequestGame::GetNumBlocks() const
{
  std::vector<const ObservableObject*> blocks;
  GetBEI().GetBlockWorld().FindLocatedMatchingObjects(*_blockworldFilter, blocks);
  
  return Util::numeric_cast<u32>( blocks.size() );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ObservableObject* ICozmoBehaviorRequestGame::GetClosestBlock() const
{
  return GetBEI().GetBlockWorld().FindMostRecentlyObservedObject( *_blockworldFilter );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID ICozmoBehaviorRequestGame::GetRobotsBlockID()
{
  if( ! _robotsBlockID.IsSet() ) {
    // set the block ID, but then leave it the same for the duration of the behavior
    const ObservableObject* closestObj = GetClosestBlock();
  
    if( closestObj != nullptr ) {
      PRINT_NAMED_DEBUG("BehaviorRequestGame.SetRobotBlockID", "%d",
                        closestObj->GetID().GetValue());
      _robotsBlockID = closestObj->GetID();
    }
  }

  // either return the valid id, or an invalid ID if we don't have a block
  return _robotsBlockID;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehaviorRequestGame::SwitchRobotsBlock()
{
  if( ! _robotsBlockID.IsSet() ) {
    // robot doesn't have a block, so just try to get it now
    return GetRobotsBlockID().IsSet();
  }

  // otherwise, try to find a new block that doesn't match this ID
  _badBlocks.insert(_robotsBlockID);

  // In this case (and only this case) we want to filter out _badBlocks, so make a new filter
  BlockWorldFilter filter( *_blockworldFilter );
  filter.SetFilterFcn( [this](const ObservableObject* obj) {
      return FilterBlocks(&GetBEI().GetProgressionUnlockComponent(),
                          &GetBEI().GetAIComponent(),
                          &GetBEI().GetRobotInfo().GetDockingComponent(),
                          obj) && 
              _badBlocks.find( obj->GetID() ) == _badBlocks.end();
    } );

  const ObservableObject* newBlock = GetBEI().GetBlockWorld().FindMostRecentlyObservedObject( filter );
  if( newBlock != nullptr ) {
    PRINT_NAMED_DEBUG("BehaviorRequestGame.SwitchRobotsBlock", "switch from %d to %d",
                      _robotsBlockID.GetValue(),
                      newBlock->GetID().GetValue());

    _robotsBlockID = newBlock->GetID();
    return true;
  }

  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehaviorRequestGame::GetLastBlockPose(Pose3d& pose) const
{
  if( _hasBlockPose ) {
    pose = _lastBlockPose;
  }
  return _hasBlockPose;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehaviorRequestGame::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  const ObservableObject* obj = GetClosestBlock();
  if( obj != nullptr ) {
    _hasBlockPose = true;
    _lastBlockPose = obj->GetPose();
  }
  
  RequestGame_UpdateInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehaviorRequestGame::HasFace() const
{
  Pose3d waste;
  return GetFacePose(waste);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ICozmoBehaviorRequestGame::GetFacePose(Pose3d& facePoseWrtRobotOrigin) const
{
  float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const u32 currTime_ms = Util::numeric_cast<u32>( std::floor( currentTime_sec * 0.001f ) );
  const bool kMustBeInRobotOrigin = true;
  
  TimeStamp_t lastObservedFaceTime = GetBEI().GetFaceWorld().GetLastObservedFace(facePoseWrtRobotOrigin, kMustBeInRobotOrigin);
  
  const bool hasFace = lastObservedFaceTime > 0 && lastObservedFaceTime + _maxFaceAge_ms > currTime_ms;

  return hasFace;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehaviorRequestGame::AlwaysHandleInScope(const EngineToGameEvent& event)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedFace:
      HandleObservedFace(event.GetData().Get_RobotObservedFace());
      break;
        
    case EngineToGameTag::RobotDeletedFace:
      HandleDeletedFace(event.GetData().Get_RobotDeletedFace());
      break;

    case EngineToGameTag::CliffEvent:
      // handled in WhileRunning
      break;

    default:
      PRINT_NAMED_WARNING("ICozmoBehaviorRequestGame.InvalidTag",
                          "Received unexpected event with tag %hhu.", event.GetData().GetTag());
      break;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehaviorRequestGame::AlwaysHandleInScope(const GameToEngineEvent& event)
{
  switch(event.GetData().GetTag())
  {
    case GameToEngineTag::DenyGameStart:
      // handled in while Running
      break;
      
    case GameToEngineTag::CanCozmoRequestGame:
      _canRequestGame = event.GetData().Get_CanCozmoRequestGame().canRequest;
      break;
      
    default:
      PRINT_NAMED_WARNING("ICozmoBehaviorRequestGame.AlwaysHandle.InvalidTag",
                          "Received unexpected event with tag %hhu.", event.GetData().GetTag());
      break;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehaviorRequestGame::HandleWhileActivated(const EngineToGameEvent& event)
{
  if( event.GetData().GetTag() == EngineToGameTag::CliffEvent ) {
    HandleCliffEvent(event);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehaviorRequestGame::HandleWhileActivated(const GameToEngineEvent& event)
{
  if( event.GetData().GetTag() == GameToEngineTag::DenyGameStart ) {
    HandleGameDeniedRequest();
    if(GetBEI().HasPublicStateBroadcaster()){
      auto& publicStateBroadcaster = GetBEI().GetRobotPublicStateBroadcaster();
      publicStateBroadcaster.UpdateRequestingGame(false);
    }
  }
  else if(event.GetData().GetTag() == GameToEngineTag::CanCozmoRequestGame){
    // Behavior game requests can no longer run - if we're running stop the behavior
    if(!event.GetData().Get_CanCozmoRequestGame().canRequest){
      CancelDelegates(false);
    }
  }
  else {
    PRINT_NAMED_WARNING("ICozmoBehaviorRequestGame.HandleWhileRunning.InvalidTag",
                        "Received unexpected event with tag %hhu.", event.GetData().GetTag());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehaviorRequestGame::HandleObservedFace(const ExternalInterface::RobotObservedFace& msg)
{
  // If faceID not already set or we're not currently tracking the update the faceID
  if (_faceID == Vision::UnknownFaceID
      || _faceID != msg.faceID) {
    _faceID = static_cast<FaceID_t>(msg.faceID);
  }

  _lastFaceSeenTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehaviorRequestGame::HandleDeletedFace(const ExternalInterface::RobotDeletedFace& msg)
{
  if (_faceID == msg.faceID) {
    _faceID = Vision::UnknownFaceID;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ICozmoBehaviorRequestGame::OnBehaviorDeactivated()
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  robotInfo.GetContext()->GetVoiceCommandComponent()->ForceListenContext(VoiceCommand::VoiceCommandListenContext::TriggerPhrase);
  
  RequestGame_OnBehaviorDeactivated();
}

} // namespace Cozmo
} // namespace Anki
