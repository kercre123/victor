/**
 * File: progressionUnlockComponent.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-04-04
 *
 * Description: A component to manage progression unlocks.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/components/unlockIdsHelpers.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "json/json.h"

// for now, fake that it takes this long to "write and confirm" the data to the robot

#define FAKE_UNLOCK_CONFIRM_DELAY_S 0.05732

namespace Anki {
namespace Cozmo {

static const char* kConfigGroupName = "unlockedFreeplayBehaviors";

ProgressionUnlockComponent::ProgressionUnlockComponent(Robot& robot)
  : _robot(robot)
{
}

void ProgressionUnlockComponent::Init(const Json::Value& inJson)
{

  Json::Value config = inJson[kConfigGroupName];
  if( config.isNull() ) {
    PRINT_NAMED_WARNING("ProgressionUnlockComponent.InvalidConfig",
                        "json config did not contain key '%s'",
                        kConfigGroupName);
  }

  // keep track of a set of freeplay behaviors to make sure there are no duplicates
  std::set<BehaviorGroup> freeplayGroups;
  
  for(UnlockId unlock = UnlockId(0); unlock < UnlockId::Count; unlock++) {
    const Json::Value& entryConfig = config[ UnlockIdToString( unlock ) ];
    ProgressionUnlockEntry entry(entryConfig);

    if(entry.HasFreeplayBehaviorGroup()) {
      if( freeplayGroups.count( entry.GetFreeplayBehaviorGroup() ) > 0 ) {
        PRINT_NAMED_ERROR("ProgressionUnlockComponent.Init.DuplicateFreeplayBehavior",
                          "Unlock '%s' has freeplay behavior group '%s' which is a duplicate",
                          UnlockIdToString(unlock),
                          BehaviorGroupToString( entry.GetFreeplayBehaviorGroup() ));
      }
      else {
        freeplayGroups.insert( entry.GetFreeplayBehaviorGroup() );
      }
    }
    
    _unlocks.insert( std::make_pair( unlock, std::move( entry ) ));
  }

  if( _robot.HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine< MessageGameToEngineTag::RequestSetUnlock >();
  }
}

bool ProgressionUnlockComponent::SetUnlock(UnlockId unlock, bool unlocked)
{
  auto it = _unlocks.find(unlock);
  if( it == _unlocks.end() ) {
    PRINT_NAMED_ERROR("ProgressionUnlockComponent.SetUnlock.UnlockNotFound",
                      "could not find unlock '%s' (%d) in map (map size %zu)",
                      UnlockIdToString(unlock),
                      (int)unlock,
                      _unlocks.size());
    return false;
  }

  it->second.SetUnlock( unlocked );

  if( it->second.HasFreeplayBehaviorGroup() )
  {
  
  // TODO rsam unlock behaviors
  
//    // if there is a freeplay behavior associated with this unlock, enable it now (if we are in the selection
//    // chooser, this will have no effect)
//    _robot.GetExternalInterface()->Broadcast( ExternalInterface::MessageGameToEngine(
//                                                ExternalInterface::BehaviorManagerMessage(
//                                                  1,
//                                                  ExternalInterface::BehaviorManagerMessageUnion(
//                                                    ExternalInterface::SetEnableBehaviorGroup(
//                                                      it->second.GetFreeplayBehaviorGroup(), unlocked)))));
  }
  
  return true;
}

bool ProgressionUnlockComponent::IsUnlocked(UnlockId unlock) const
{
  const auto it = _unlocks.find(unlock);
  if( it == _unlocks.end() ) {
    PRINT_NAMED_ERROR("ProgressionUnlockComponent.IsUnlocked.UnlockNotFound",
                      "could not find unlock '%s' (%d) in map (map size %zu)",
                      UnlockIdToString(unlock),
                      (int)unlock,
                      _unlocks.size());
    return false;
  }
  return it->second.IsUnlocked();
}

void ProgressionUnlockComponent::SendUnlockStatus() const
{
  if( ! _robot.HasExternalInterface() ) {
    return;
  }

  std::vector<ExternalInterface::UnlockEntry> unlocks;
  for( const auto& unlock : _unlocks ) {
    unlocks.emplace_back( ExternalInterface::UnlockEntry{ unlock.first, unlock.second.IsUnlocked() } );
  }

  _robot.GetExternalInterface()->Broadcast( ExternalInterface::MessageEngineToGame(
                                              ExternalInterface::UnlockStatus(
                                                unlocks)));
}

void ProgressionUnlockComponent::IterateUnlockedFreeplayBehaviors(
  ProgressionUnlockComponent::UnlockBehaviorCallback callback)
{
  for( const auto& unlock : _unlocks ) {
    if( unlock.second.HasFreeplayBehaviorGroup() ) {
      callback(unlock.second.GetFreeplayBehaviorGroup(), unlock.second.IsUnlocked());
    }
  }
}

void ProgressionUnlockComponent::Update()
{
  float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  BOUNDED_WHILE( 100, ! _confirmationsToSend.empty() ) {
    auto top = _confirmationsToSend.begin();
    if( currTime > top->first ) {
      _robot.GetExternalInterface()->Broadcast( ExternalInterface::MessageEngineToGame(
                                                  ExternalInterface::RequestSetUnlockResult(
                                                    top->second, IsUnlocked(top->second))));
      _confirmationsToSend.erase( top );
    }
    else {
      break;
    }
  }
}

template<>
void ProgressionUnlockComponent::HandleMessage(const ExternalInterface::RequestSetUnlock& msg)
{
  bool ret = SetUnlock(msg.unlockID, msg.unlocked);
  if( ret ) {
    // wait a bit before broadcasting result, to simulate time it might take for the robot to write and verify
    // the data
    float timeToUnlock = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + FAKE_UNLOCK_CONFIRM_DELAY_S;
    _confirmationsToSend.insert( std::make_pair( timeToUnlock, msg.unlockID ) );
  }
}

}
}
