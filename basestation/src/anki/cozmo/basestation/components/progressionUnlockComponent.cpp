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
#include "util/helpers/templateHelpers.h"
#include "json/json.h"

// for now, fake that it takes this long to "write and confirm" the data to the robot

#define FAKE_UNLOCK_CONFIRM_DELAY_S 0.05732

namespace Anki {
namespace Cozmo {

ProgressionUnlockComponent::ProgressionUnlockComponent(Robot& robot)
  : _robot(robot)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ProgressionUnlockComponent::Init()
{
  // todo rsam remove Json if we don't need it

  // register to get unlock requests
  if( _robot.HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine< MessageGameToEngineTag::RequestSetUnlock >();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ProgressionUnlockComponent::SetUnlock(UnlockId unlock, bool unlocked)
{
  bool success = false;

  // depending on whether we want to set/unset the unlock
  if ( unlocked )
  {
    // we want to unlock (= add to the set of unlocked)
    auto ret = _currentUnlocks.insert(unlock);
    
    // debug the result
    if ( ret.second ) {
      // successfully unlocked
      success = true;
      PRINT_NAMED_DEBUG("ProgressionUnlockComponent.SetUnlock.Unlocked",
                        "Unlocked '%s' (%d)",
                        UnlockIdToString(unlock), (int)unlock);
    } else {
      // already unlocked
      PRINT_NAMED_ERROR("ProgressionUnlockComponent.SetUnlock.DuplicatedUnlock",
                        "Tried to unlock '%s' (%d), but it's already unlocked",
                        UnlockIdToString(unlock), (int)unlock);
    }
  }
  else
  {
    // we want to lock back (= remove from the set)
    const size_t erasedCount = _currentUnlocks.erase(unlock);
    const bool successfullyLocked = erasedCount > 0;
    if ( successfullyLocked )
    {
      // successfully locked
      success = true;
      PRINT_NAMED_DEBUG("ProgressionUnlockComponent.SetUnlock.Locked",
                        "Locked '%s' (%d)",
                        UnlockIdToString(unlock), (int)unlock);
    }
    else
    {
      // already locked
      PRINT_NAMED_ERROR("ProgressionUnlockComponent.SetUnlock.MissingUnlock",
                        "Tried to lock '%s' (%d), but it was already locked",
                        UnlockIdToString(unlock), (int)unlock);
    }
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ProgressionUnlockComponent::IsUnlocked(UnlockId unlock) const
{
  const auto matchIt = _currentUnlocks.find(unlock);
  const bool isUnlocked = (matchIt != _currentUnlocks.end());
  return isUnlocked;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ProgressionUnlockComponent::SendUnlockStatus() const
{
  if( ! _robot.HasExternalInterface() ) {
    return;
  }

  // vector we are going to send
  std::vector<ExternalInterface::UnlockEntry> allUnlocks;

  // fill the vector with entries from 0 to count-1 all set to false
  using EnumSize_t = std::underlying_type<UnlockId>::type;
  EnumSize_t unlockTypeCount = Util::EnumToUnderlying( UnlockId::Count );
  allUnlocks.reserve(unlockTypeCount);
  for(EnumSize_t typeIt=0; typeIt<unlockTypeCount; ++typeIt) {
    allUnlocks.emplace_back( (UnlockId)typeIt, false );
  }
  
  // iterate actually unlocked ones and set their entry flag to true
  for( const auto& unlockIt : _currentUnlocks ) {
    allUnlocks[ Util::EnumToUnderlying( unlockIt ) ].unlocked = true;
  }

  // now send
  _robot.GetExternalInterface()->Broadcast( ExternalInterface::MessageEngineToGame(
                                              ExternalInterface::UnlockStatus(
                                                allUnlocks)));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
