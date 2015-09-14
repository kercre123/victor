/**
 * File: uiMessageHandler.cpp
 *
 * Author: Kevin Yoon
 * Date:   7/11/2014
 *
 * Description: Handles messages between UI and basestation just as
 *              RobotMessageHandler handles messages between basestation and robot.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "util/logging/logging.h"

#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/game/comms/uiMessageHandler.h"
#include "anki/cozmo/basestation/soundManager.h"

#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/cozmoActions.h"

#include "anki/cozmo/basestation/viz/vizManager.h"

#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/utils/timer.h"

#if(RUN_UI_MESSAGE_TCP_SERVER)
#include "anki/cozmo/shared/cozmoConfig.h"
#else
#include "anki/cozmo/basestation/ui/messaging/messageQueue.h"
#endif

namespace Anki {
  namespace Cozmo {

    UiMessageHandler::UiMessageHandler(u32 hostUiDeviceID)
    : comms_(NULL)
    , isInitialized_(false)
    , _hostUiDeviceID(hostUiDeviceID)
    {
      
    }
    Result UiMessageHandler::Init(Comms::IComms*   comms)
    {
      Result retVal = RESULT_FAIL;
      
      if(comms != nullptr) {
        comms_ = comms;
        
        isInitialized_ = true;
        retVal = RESULT_OK;
      }
      
      return retVal;
    }

    void UiMessageHandler::DeliverToGame(const ExternalInterface::MessageEngineToGame& message)
    {
#if(RUN_UI_MESSAGE_TCP_SERVER)

      Comms::MsgPacket p;
      message.Pack(p.data, Comms::MsgPacket::MAX_SIZE);
      p.dataLen = message.Size();
      p.destId = _hostUiDeviceID;
      comms_->Send(p);
#else
      //MessageQueue::getInstance()->AddMessageForUi(msg);
#endif
    }
  
    Result UiMessageHandler::ProcessPacket(const Comms::MsgPacket& packet)
    {
      Result retVal = RESULT_FAIL;
      
      ExternalInterface::MessageGameToEngine message;
      if (message.Unpack(packet.data, Comms::MsgPacket::MAX_SIZE) != packet.dataLen) {
        PRINT_STREAM_ERROR("UiMessageHandler.MessageBufferWrongSize",
                          "Buffer's size does not match expected size for this message ID. (Msg " << ExternalInterface::MessageGameToEngineTagToString(message.GetTag()) << ", expected " << message.Size() << ", recvd " << packet.dataLen << ")");
      }
      
      if (messageCallback != nullptr) {
        messageCallback(message);
      }
      
      return retVal;
    } // ProcessBuffer()
    
    // Broadcasting MessageGameToEngine messages are only internal
    void UiMessageHandler::Broadcast(const ExternalInterface::MessageGameToEngine& message)
    {
      _eventMgrToEngine.Broadcast(AnkiEvent<ExternalInterface::MessageGameToEngine>(
        BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), static_cast<u32>(message.GetTag()), message));
    } // Broadcast(MessageGameToEngine)
    
    void UiMessageHandler::Broadcast(ExternalInterface::MessageGameToEngine&& message)
    {
      u32 type = static_cast<u32>(message.GetTag());
      _eventMgrToEngine.Broadcast(AnkiEvent<ExternalInterface::MessageGameToEngine>(
        BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), type, std::move(message)));
    } // Broadcast(MessageGameToEngine &&)
    
    
    // Broadcasting MessageEngineToGame messages also delivers them out of the engine
    void UiMessageHandler::Broadcast(const ExternalInterface::MessageEngineToGame& message)
    {
      DeliverToGame(message);
      _eventMgrToGame.Broadcast(AnkiEvent<ExternalInterface::MessageEngineToGame>(
        BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), static_cast<u32>(message.GetTag()), message));
    } // Broadcast(MessageEngineToGame)
    
    void UiMessageHandler::Broadcast(ExternalInterface::MessageEngineToGame&& message)
    {
      DeliverToGame(message);
      u32 type = static_cast<u32>(message.GetTag());
      _eventMgrToGame.Broadcast(AnkiEvent<ExternalInterface::MessageEngineToGame>(
        BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), type, std::move(message)));
    } // Broadcast(MessageEngineToGame &&)
    
    // Provides a way to subscribe to message types using the AnkiEventMgrs
    Signal::SmartHandle UiMessageHandler::Subscribe(const ExternalInterface::MessageEngineToGameTag& tagType,
                                                    std::function<void(const AnkiEvent<ExternalInterface::MessageEngineToGame>&)> messageHandler)
    {
      return _eventMgrToGame.Subcribe(static_cast<u32>(tagType), messageHandler);
    } // Subscribe(MessageEngineToGame)
    
    Signal::SmartHandle UiMessageHandler::Subscribe(const ExternalInterface::MessageGameToEngineTag& tagType,
                                                    std::function<void(const AnkiEvent<ExternalInterface::MessageGameToEngine>&)> messageHandler)
    {
      return _eventMgrToEngine.Subcribe(static_cast<u32>(tagType), messageHandler);
    } // Subscribe(MessageGameToEngine)

    
    Result UiMessageHandler::ProcessMessages()
    {
      Result retVal = RESULT_FAIL;
      
      if(isInitialized_) {
        retVal = RESULT_OK;
        
        while(comms_->GetNumPendingMsgPackets() > 0)
        {
          Comms::MsgPacket packet;
          comms_->GetNextMsgPacket(packet);
          
          if(ProcessPacket(packet) != RESULT_OK) {
            retVal = RESULT_FAIL;
          }
        } // while messages are still available from comms
      }
      
      return retVal;
    } // ProcessMessages()
    
  } // namespace Cozmo
} // namespace Anki
