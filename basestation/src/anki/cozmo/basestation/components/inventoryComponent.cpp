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

#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/components/inventoryComponent.h"
#include "anki/cozmo/basestation/components/nvStorageComponent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"


#define CONSOLE_GROUP_NAME "InventoryComponent"

namespace Anki {
namespace Cozmo {
  
static const int kMinimumTimeBetweenRobotSaves_sec = 5;


InventoryComponent::InventoryComponent(Robot& robot)
  : _robot(robot)
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
void InventoryComponent::Init()
{
  ReadCurrentInventoryFromRobot();
  
  // Subscribe to messages
  if (_robot.HasExternalInterface())
  {
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::InventoryRequestAdd>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::InventoryRequestSet>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::InventoryRequestGet>();
  }
}
  
void InventoryComponent::Update(const float currentTime_s)
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
}
  
int  InventoryComponent::GetInventoryAmount(InventoryType inventoryID)
{
  return _currentInventory.inventoryItemAmount[static_cast<size_t>(inventoryID)];
}
  
void InventoryComponent::SendInventoryAllToGame()
{
  if( _readFromRobot && _robot.HasExternalInterface())
  {
    ExternalInterface::InventoryStatus message(_currentInventory);
    _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
    
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
  if(!_robot.GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_InventoryData,
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
                                            // wiped so assume 0, inited in constructor.
                                            SendInventoryAllToGame();
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
  
  if(!_robot.GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_InventoryData, buf, numBytes,
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
