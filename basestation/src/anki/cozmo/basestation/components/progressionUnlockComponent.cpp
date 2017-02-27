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

#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/components/nvStorageComponent.h"
#include "anki/cozmo/basestation/components/unlockIdsHelpers.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "json/json.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/helpers/templateHelpers.h"

namespace Anki {
namespace Cozmo {

static std::set<UnlockId> _defaultUnlocks;

static const char* kDefaultUnlockIdsConfigKey = "defaultUnlocks";
static const char* kFreeplayOverridesKey = "freeplayOverrides";

CONSOLE_VAR(u32, kNumAttemptsToWrite, "ProgressionUnlockComponent", 5);

ProgressionUnlockComponent::ProgressionUnlockComponent(Robot& robot)
  : _robot(robot)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ProgressionUnlockComponent::Init(const Json::Value &config)
{
  _defaultUnlocks.clear();
  
  // load default unlocks from json
  if( !config.isNull() && config[kDefaultUnlockIdsConfigKey].isArray() ) {
    for( const auto& unlockIdJson : config[kDefaultUnlockIdsConfigKey] ) {
      if( ! unlockIdJson.isString() ) {
        PRINT_NAMED_ERROR("ProgressionUnlockComponent.Init.InvalidDefaultData",
                          "invalid element in unlock id's list (not a string)");
        continue;
      }
      
      UnlockId uid = UnlockIdsFromString(unlockIdJson.asString());
      _defaultUnlocks.insert(uid);

      PRINT_CH_INFO("UnlockComponent", "ProgressionUnlockComponent.DefaultValue",
                    "%s defaults to true",
                    UnlockIdToString(uid));
    }
  }
  else {
    PRINT_NAMED_ERROR("ProgressionUnlockComponent.Init.MissingDefaults",
                      "missing key '%s'",
                      kDefaultUnlockIdsConfigKey);
  }

  _freeplayOverrides.clear();

  // load freeplay overrides from json
  if( !config.isNull() && config[kFreeplayOverridesKey].isArray() ) {
    for( const auto& unlockIdJson : config[kFreeplayOverridesKey] ) {
      if( ! unlockIdJson.isString() ) {
        PRINT_NAMED_ERROR("ProgressionUnlockComponent.Init.InvalidOverrideData",
                          "invalid element in unlock id's list (not a string)");
        continue;
      }
      
      UnlockId uid = UnlockIdsFromString(unlockIdJson.asString());
      _freeplayOverrides.insert(uid);

      PRINT_CH_INFO("UnlockComponent", "ProgressionUnlockComponent.Override",
                    "%s will be overridden in freeplay to true",
                    UnlockIdToString(uid));
    }
  }
  else {
    PRINT_NAMED_ERROR("ProgressionUnlockComponent.Init.MissingOverrides",
                      "missing key '%s'",
                      kFreeplayOverridesKey);
  }

  ReadCurrentUnlocksFromRobot();
  
  // register to get unlock requests
  if( _robot.HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine< MessageGameToEngineTag::RequestSetUnlock >();
  }
}

const std::set<UnlockId>& ProgressionUnlockComponent::GetDefaultUnlocks()
{
  return _defaultUnlocks;
}

bool ProgressionUnlockComponent::IsUnlockIdValid(UnlockId id)
{
  if(id == UnlockId::All ||
     id == UnlockId::Defaults ||
     id <= UnlockId::Invalid ||
     id >= UnlockId::Count ||
     EnumToString(id) == nullptr)
  {
    return false;
  }
  return true;
}
  
void ProgressionUnlockComponent::NotifyGameDefaultUnlocksSet()
{
  // Let unity know what the default unlocks were.
  _robot.GetExternalInterface()->Broadcast( ExternalInterface::MessageEngineToGame(
                                               ExternalInterface::UnlockedDefaults(std::vector<UnlockId>(_defaultUnlocks.begin(), _defaultUnlocks.end()))));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ProgressionUnlockComponent::SetUnlock(UnlockId unlock, bool unlocked)
{
  bool success = false;
  
  if(unlock == UnlockId::All)
  {
    for(u8 i = (u8)UnlockId::Invalid; i < (u8)UnlockId::Count; ++i)
    {
      if(IsUnlockIdValid((UnlockId)i))
      {
        SetUnlock((UnlockId)i, unlocked);
      }
    }
  }
  else if(unlock == UnlockId::Defaults)
  {
    for(u8 i = (u8)UnlockId::Invalid; i < (u8)UnlockId::Count; ++i)
    {
      if(IsUnlockIdValid((UnlockId)i))
      {
        SetUnlock((UnlockId)i, (_defaultUnlocks.find((UnlockId)i) != _defaultUnlocks.end()));
      }
    }
    NotifyGameDefaultUnlocksSet();
  }
  else if(IsUnlockIdValid(unlock))
  {
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
        PRINT_CH_INFO("UnlockComponent", "ProgressionUnlockComponent.SetUnlock.DuplicatedUnlock",
                      "Tried to unlock '%s' (%d), but it's already unlocked (dev button?)",
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
        PRINT_CH_INFO("UnlockComponent", "ProgressionUnlockComponent.SetUnlock.MissingUnlock",
                      "Tried to lock '%s' (%d), but it was already locked (dev button?)",
                      UnlockIdToString(unlock), (int)unlock);
      }
    }
  }
  
  if(success)
  {
    WriteCurrentUnlocksToRobot(unlock, unlocked);
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ProgressionUnlockComponent::IsUnlocked(UnlockId unlock, bool forFreeplay) const
{
  if (_robot.GetContext()->IsInSdkMode())
  {
    // Progression is irrelevant in sdk mode - just force everything unlocked
    return true;
  }

  if( forFreeplay ) {
    // check overrides to see if we should lie about this unlock
    const auto overrideIt = _freeplayOverrides.find(unlock);
    const bool overrideReturnTrue = (overrideIt != _freeplayOverrides.end());
    if( overrideReturnTrue ) {
      return true;
    }
  }
  
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

  PRINT_CH_INFO("UnlockComponent",
                "SendUnlockStatus",
                "Sending current unlock status (%zu unlocked)",
                _currentUnlocks.size());
  
  // now send
  _robot.GetExternalInterface()->Broadcast( ExternalInterface::MessageEngineToGame(
                                              ExternalInterface::UnlockStatus(
                                                std::vector<UnlockId>(_currentUnlocks.begin(), _currentUnlocks.end()), false)));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ProgressionUnlockComponent::Update()
{

}

void ProgressionUnlockComponent::WriteCurrentUnlocksToRobot(UnlockId id, bool unlocked)
{
  ANKI_CPU_PROFILE("ProgressionUnlockComponent::WriteCurrentUnlocksToRobot");
  
  UnlockedIdsList unlockedIds;
  unlockedIds.unlockedIds.fill(UnlockId::Invalid);
  int index = 0;
  for(const UnlockId id : _currentUnlocks)
  {
    unlockedIds.unlockedIds[index++] = id;
  }
  
  u8 buf[unlockedIds.Size()];
  size_t numBytes = unlockedIds.Pack(buf, sizeof(buf));
  
  if(!_robot.GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_GameUnlocks, buf, numBytes,
                                          [this, id, unlocked](NVStorage::NVResult res)
                                          {
                                            if (res < NVStorage::NVResult::NV_OKAY)
                                            {
                                              PRINT_NAMED_ERROR("UnlockComponent.WriteCurrentUnlocksToRobot,",
                                                            "Write failed with %s",
                                                            EnumToString(res));
                                            }
                                            else
                                            {
                                              // The write was successful so broadcast the unlock result
                                              if(_robot.HasExternalInterface())
                                              {
                                                PRINT_CH_INFO("UnlockComponent",
                                                              "WriteCurrentUnlocksToRobot",
                                                              "Broadcasting SetUnlockResult for %s : %s",
                                                              EnumToString(id),
                                                              (unlocked ? "unlocked" : "locked"));
                                                _robot.GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RequestSetUnlockResult(id, unlocked)));
                                              }
                                            }
                                          }))
  {
    PRINT_CH_INFO("UnlockComponent",
                  "WriteCurrentUnlocksToRobot",
                  "Write failed");
  }
}

void ProgressionUnlockComponent::ReadCurrentUnlocksFromRobot()
{
  if(!_robot.GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_GameUnlocks,
                                          [this](u8* data, size_t size, NVStorage::NVResult res)
                                          {
                                            if(res < NVStorage::NVResult::NV_OKAY)
                                            {
                                              // The tag doesn't exist on the robot indicating the robot is new or has been
                                              // wiped so we need to unlock the default unlocks
                                              if(res == NVStorage::NVResult::NV_NOT_FOUND)
                                              {
                                                PRINT_CH_INFO("UnlockComponent",
                                                              "ReadGameUnlocks",
                                                              "No unlock data found on robot unlocking defaults");
                                                
                                                _currentUnlocks = _defaultUnlocks;
                                                
                                                SendUnlockStatus();
                                                NotifyGameDefaultUnlocksSet();
                                                
                                              }
                                              // Otherwise something went wrong with reading from the robot so try again
                                              else
                                              {
                                                PRINT_NAMED_ERROR("UnlockComponent.ReadGameUnlocks",
                                                              "Read failed with %s",
                                                              EnumToString(res));
                                              }
                                            }
                                            else
                                            {
                                              UnlockedIdsList unlockedIds;
                                              
                                              if(size < NVStorageComponent::MakeWordAligned(unlockedIds.Size()))
                                              {
                                                PRINT_NAMED_INFO("ProgressionUnlockComponent.ReadFewerUnlocks",
                                                                 "Padding unlock data due to size mismatch");
                                                
                                                std::vector<u8> padded = std::vector<u8>(data, data + size);
                                                
                                                for(int i = 0; i < unlockedIds.Size() - size; ++i)
                                                {
                                                  padded.insert(padded.end(), 0);
                                                }
                                                
                                                DEV_ASSERT(unlockedIds.Size() == padded.size(),
                                                           "unlockIds and padded not equal in size");
                                                
                                                size = unlockedIds.Size();
                                                data = padded.data();
                                              }
                                              
                                              if(size != NVStorageComponent::MakeWordAligned(unlockedIds.Size()))
                                              {
                                                PRINT_NAMED_ERROR("UnlockComponent.ReadGameUnlocks",
                                                                  "Size mismatch expected %zu, got %zu",
                                                                  NVStorageComponent::MakeWordAligned(unlockedIds.Size()),
                                                                  size);
                                              }
                                              else
                                              {
                                                unlockedIds.Unpack(data, size);
                                                _currentUnlocks.clear();
                                                
                                                for(const UnlockId& id : unlockedIds.unlockedIds)
                                                {
                                                  if(id > UnlockId::Invalid && id < UnlockId::Count)
                                                  {
                                                    _currentUnlocks.insert(id);
                                                  }
                                                }
                                                
                                                SendUnlockStatus();
                                              }
                                            }
                                          }))
  {
    PRINT_CH_INFO("UnlockComponent",
                  "ReadCurrentUnlocksToRobot",
                  "Read failed");
  }
}

template<>
void ProgressionUnlockComponent::HandleMessage(const ExternalInterface::RequestSetUnlock& msg)
{
  LOG_EVENT("meta.unlock.engineHandle","%s %d",EnumToString(msg.unlockID), msg.unlocked);
  if(msg.unlockID <= UnlockId::Invalid ||
     msg.unlockID >= UnlockId::Count)
  {
    PRINT_CH_INFO("UnlockComponent",
                  "HandleRequestSetUnlock",
                  "Invalid unlockId %d, ignoring",
                  msg.unlockID);
    return;
  }
  SetUnlock(msg.unlockID, msg.unlocked);
}

}
}
