/**
 * File: inventoryComponent.cpp
 *
 * Author: Molly Jameson
 * Created: 2017-05-24
 *
 * Description: A component to manage inventory
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "coretech/common/engine/jsonTools.h"
#include "engine/ankiEventUtil.h"
#include "engine/components/inventoryComponent.h"
#include "engine/components/nvStorageComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"


#define CONSOLE_GROUP_NAME "InventoryComponent"

namespace Anki {
namespace Cozmo {

static const int kMinimumTimeBetweenRobotSaves_sec = 5;
static const std::string kInventoryTypeCapsConfigKey = "InventoryTypeCaps";

InventoryComponent::InventoryComponent()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::Inventory)
, _readFromRobot(false)
, _timeLastWrittenToRobot(std::chrono::system_clock::now())
, _robotWritePending(false)
, _isWritingToRobot(false)
{
  // Default onboarding values initted by unity config to match old API where sparks were on device.
  for( int i = 0; i < InventoryTypeNumEntries; ++i )
  {
    _currentInventory.inventoryItemAmount[i] = 0;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InventoryComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;
  ReadCurrentInventoryFromRobot();

  // Subscribe to messages
  if (_robot->HasExternalInterface())
  {
    auto helper = MakeAnkiEventUtil(*_robot->GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::InventoryRequestAdd>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::InventoryRequestSet>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::InventoryRequestGet>();
  }

  auto& context = dependentComponents.GetValue<ContextWrapper>().context;

  if (nullptr != context->GetDataPlatform())
  {
    ReadInventoryConfig(context->GetDataLoader()->GetInventoryConfig());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InventoryComponent::ReadInventoryConfig(const Json::Value& config)
{
  // parse inventory caps from json
  const Json::Value& inventoryTypeCaps = config[kInventoryTypeCapsConfigKey];
  for (int i = 0; i < InventoryTypeNumEntries; i++)
  {
    InventoryType inventoryID = (InventoryType)i;
    const Json::Value& cap = inventoryTypeCaps[InventoryTypeToString(inventoryID)];
    if (!cap.isNull())
    {
      _inventoryTypeCaps[inventoryID] = JsonTools::GetValue<int>(cap);
    }
  }
}

void InventoryComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  if( !_isWritingToRobot && _robotWritePending)
  {
    TryWriteCurrentInventoryToRobot();
  }
}

void InventoryComponent::SetInventoryAmount(InventoryType inventoryID, int total)
{
  PRINT_CH_INFO(CONSOLE_GROUP_NAME,"InventoryComponent.SetInventoryAmount","%s : %d",
                InventoryTypeToString(inventoryID),GetInventoryAmount(inventoryID));
  if( _readFromRobot )
  {
    const int cap = GetInventoryCap(inventoryID);
    if (cap != kInfinity && total > cap)
    {
      total = cap;
    }
    _currentInventory.inventoryItemAmount[static_cast<size_t>(inventoryID)] = total;
    TryWriteCurrentInventoryToRobot();
  }
  else
  {
    // Keep this as a debug for now because it is a valid thing to do in the old homehub
    // TODO: bump up to warning when switch over fully to needs view
    PRINT_CH_DEBUG(CONSOLE_GROUP_NAME,"InventoryComponent.SetInventoryAmount.ComponentNotReady","");
  }
}

void InventoryComponent::AddInventoryAmount(InventoryType inventoryID, int delta)
{
  SetInventoryAmount(inventoryID,GetInventoryAmount(inventoryID) + delta);

  const std::string inventoryType = (inventoryID == InventoryType::Sparks ? "spark" : "");

  // DAS Event: "meta.inventory.change"
  // s_val: Sparks inventory type
  // data: Change (delta) in count
  Anki::Util::sInfo("meta.inventory.change",
                    {{DDATA, std::to_string(delta).c_str()}},
                    inventoryType.c_str());

  // DAS Event: "meta.inventory.balance"
  // s_val: Sparks inventory type
  // data: New inventory balance
  Anki::Util::sInfo("meta.inventory.balance",
                    {{DDATA, std::to_string(GetInventoryAmount(inventoryID)).c_str()}},
                    inventoryType.c_str());
}

int  InventoryComponent::GetInventoryAmount(InventoryType inventoryID) const
{
  return _currentInventory.inventoryItemAmount[static_cast<size_t>(inventoryID)];
}

int InventoryComponent::GetInventoryCap(InventoryType inventoryID) const
{
  return _inventoryTypeCaps.find(inventoryID) != _inventoryTypeCaps.end()
    ? _inventoryTypeCaps.at(inventoryID)
    : kInfinity;
}

int InventoryComponent::GetInventorySpaceRemaining(InventoryType inventoryID) const
{
  const int cap = GetInventoryCap(inventoryID);
  const int currentAmount = GetInventoryAmount(inventoryID);

  if (cap == kInfinity)
  {
    return kInfinity;
  }

  return (currentAmount < cap)
    ? cap - currentAmount
    : 0;
}

void InventoryComponent::SendInventoryAllToGame()
{
  if( _readFromRobot && _robot->HasExternalInterface())
  {
    ExternalInterface::InventoryStatus message(_currentInventory);
    _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));

#if ANKI_DEV_CHEATS
    for( int i = 0; i < InventoryTypeNumEntries; ++i )
    {
      InventoryType inventoryID = (InventoryType)i;
      if( inventoryID != InventoryType::Invalid )
      {
        PRINT_CH_INFO(CONSOLE_GROUP_NAME,"InventoryComponent.SendInventoryItemToGame","%s : %d",
                      InventoryTypeToString(inventoryID),GetInventoryAmount(inventoryID));
      }
    }
#endif

  }
}

template<>
void InventoryComponent::HandleMessage(const ExternalInterface::InventoryRequestAdd& msg)
{
  AddInventoryAmount(msg.inventoryType, msg.delta);
}

template<>
void InventoryComponent::HandleMessage(const ExternalInterface::InventoryRequestSet& msg)
{
  SetInventoryAmount(msg.inventoryType, msg.total);
}

template<>
void InventoryComponent::HandleMessage(const ExternalInterface::InventoryRequestGet& msg)
{
  SendInventoryAllToGame();
}

void InventoryComponent::ReadCurrentInventoryFromRobot()
{
  PRINT_CH_INFO(CONSOLE_GROUP_NAME,"InventoryComponent.ReadCurrentInventoryFromRobot","");
  if(!_robot->GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_InventoryData,
                                          [this](u8* data, size_t size, NVStorage::NVResult res)
                                          {
                                            _readFromRobot = true;
                                            if(res >= NVStorage::NVResult::NV_OKAY)
                                            {
                                              _currentInventory.Unpack(data,size);
                                            }
                                            else if(res < NVStorage::NVResult::NV_OKAY && res != NVStorage::NVResult::NV_NOT_FOUND)
                                            {
                                              PRINT_NAMED_ERROR("InventoryComponent.ReadCurrentInventoryFromRobot.ReadError","");
                                            }
                                            // If the tag doesn't exist on the robot indicating the robot is new or has been
                                            // wiped so assume 0, initialized in constructor.
                                            SendInventoryAllToGame();

                                            // Unity has the default inventory for sparks, so request and it will get added
                                            if( res == NVStorage::NVResult::NV_NOT_FOUND && _robot->HasExternalInterface())
                                            {
                                              ExternalInterface::RequestDefaultSparks message;
                                              _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
                                            }
                                          }))
  {
    PRINT_CH_INFO(CONSOLE_GROUP_NAME,"InventoryComponent.ReadCurrentInventoryFromRobot.ReadFailed","");
  }
}

// Realistically we're not going to have that many inventory message,
// but throttle just in case
void InventoryComponent::TryWriteCurrentInventoryToRobot()
{
  const std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
  const auto elapsed = now - _timeLastWrittenToRobot;
  const auto secsSinceLastSave = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
  if ( secsSinceLastSave < kMinimumTimeBetweenRobotSaves_sec)
  {
    _robotWritePending = true;
    PRINT_CH_INFO(CONSOLE_GROUP_NAME,"InventoryComponent.TryWriteCurrentInventoryToRobot.QueueNextWrite","");
  }
  else
  {
    _timeLastWrittenToRobot = now;
    WriteCurrentInventoryToRobot();
  }
}

void InventoryComponent::WriteCurrentInventoryToRobot()
{
  _isWritingToRobot = true;
  // If we're throttled how do we get a callback later?
  PRINT_CH_INFO(CONSOLE_GROUP_NAME,"InventoryComponent.WriteCurrentInventoryToRobot","");
  u8 buf[_currentInventory.Size()];
  size_t numBytes = _currentInventory.Pack(buf, sizeof(buf));

  if(!_robot->GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_InventoryData, buf, numBytes,
                                           [this](NVStorage::NVResult res)
                                           {
                                             _robotWritePending = false;
                                             _isWritingToRobot = false;
                                             if (res < NVStorage::NVResult::NV_OKAY)
                                             {
                                               PRINT_NAMED_ERROR("InventoryComponent.WriteCurrentInventoryToRobot.WriteFailed",
                                                                 "Write failed with %s",
                                                                 EnumToString(res));
                                             }
                                             else
                                             {
                                               // The write was successful so broadcast the result
                                               SendInventoryAllToGame();
                                             }
                                           }))
  {
    PRINT_CH_INFO(CONSOLE_GROUP_NAME,"InventoryComponent.WriteCurrentInventoryToRobot.WriteFailed","");
    _isWritingToRobot = false;
  }
}

}
}
