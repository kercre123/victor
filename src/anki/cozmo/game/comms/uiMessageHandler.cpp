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
#include "anki/cozmo/game/signals/cozmoGameSignals.h"
#include "anki/cozmo/basestation/soundManager.h"

#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/cozmoActions.h"

#include "anki/cozmo/basestation/viz/vizManager.h"

#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/point_impl.h"

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
    

    Result UiMessageHandler::SendMessage(const ExternalInterface::MessageEngineToGame& msg)
    {
      #if(RUN_UI_MESSAGE_TCP_SERVER)
      
      Comms::MsgPacket p;
      msg.Pack(p.data, Comms::MsgPacket::MAX_SIZE);
      p.dataLen = msg.Size();
      p.destId = _hostUiDeviceID;
      
      return comms_->Send(p) > 0 ? RESULT_OK : RESULT_FAIL;
      
      #else
      
      //MessageQueue::getInstance()->AddMessageForUi(msg);
      
      #endif
      
      return RESULT_OK;
    }

    void UiMessageHandler::DeliverToGame(const ExternalInterface::MessageEngineToGame&& message)
    {
#if(RUN_UI_MESSAGE_TCP_SERVER)

      Comms::MsgPacket p;
      message.Pack(p.data, Comms::MsgPacket::MAX_SIZE);
      p.dataLen = message.Size();
      p.destId = _hostUiDeviceID;
      comms_->Send(p) > 0 ? RESULT_OK : RESULT_FAIL;
#else
      //MessageQueue::getInstance()->AddMessageForUi(msg);
#endif
    }
  
    Result UiMessageHandler::ProcessPacket(const Comms::MsgPacket& packet)
    {
      Result retVal = RESULT_FAIL;
      
      U2G::Message message;
      if (message.Unpack(packet.data, Comms::MsgPacket::MAX_SIZE) != packet.dataLen) {
        PRINT_STREAM_ERROR("UiMessageHandler.MessageBufferWrongSize",
                          "Buffer's size does not match expected size for this message ID. (Msg " << U2G::MessageTagToString(message.GetTag()) << ", expected " << message.Size() << ", recvd " << packet.dataLen << ")");
      }
      
      if (messageCallback != nullptr) {
        messageCallback(message);
      }
      
      return retVal;
    } // ProcessBuffer()
    
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
