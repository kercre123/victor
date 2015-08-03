#include "anki/cozmo/game/comms/gameMessageHandler.h"

#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

  GameMessageHandler::GameMessageHandler()
  : comms_(NULL)
  , isInitialized_(false)
  {
    
  }
  
  Result GameMessageHandler::Init(Comms::IComms* comms)
  {
    Result retVal = RESULT_FAIL;
    
    //TODO: PRINT_NAMED_DEBUG("MessageHandler", "Initializing comms");
    comms_ = comms;
    
    if(comms_) {
      isInitialized_ = comms_->IsInitialized();
      if (isInitialized_ == false) {
        // TODO: PRINT_NAMED_ERROR("MessageHandler", "Unable to initialize comms!");
        retVal = RESULT_OK;
      }
    }
    
    return retVal;
  }
  
  bool GameMessageHandler::IsInitialized() const {
    return isInitialized_;
  }
  
  
      Result GameMessageHandler::SendMessage(const UserDeviceID_t devID, const ExternalInterface::MessageGameToEngine& msg)
      {
//#if(RUN_UI_MESSAGE_TCP_SERVER)
        
        Comms::MsgPacket p;
        msg.Pack(p.data, Comms::MsgPacket::MAX_SIZE);
        p.dataLen = msg.Size();
        p.destId = devID;
        
        return comms_->Send(p) > 0 ? RESULT_OK : RESULT_FAIL;
        
//#else
        
        //MessageQueue::getInstance()->AddMessageForUi(msg);
        
//#endif
        
        return RESULT_OK;
      }
      
      
      Result GameMessageHandler::ProcessPacket(const Comms::MsgPacket& packet)
      {
        Result retVal = RESULT_FAIL;

        ExternalInterface::MessageEngineToGame message;
        if (message.Unpack(packet.data, Comms::MsgPacket::MAX_SIZE) != packet.dataLen) {
          PRINT_STREAM_ERROR("GameMessageHandler.MessageBufferWrongSize",
            "Buffer's size does not match expected size for this message ID. (Msg " <<
            ExternalInterface::MessageEngineToGameTagToString(message.GetTag()) << ", expected " << message.Size() <<
            ", recvd " << packet.dataLen << ")");
        }

        if (messageCallback != nullptr) {
          messageCallback(message);
        }
        
        return retVal;
      } // ProcessBuffer()
      
      Result GameMessageHandler::ProcessMessages()
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
} // namepsace Anki