#include "anki/cozmo/basestation/game/gameMessageHandler.h"

#include "anki/common/basestation/utils/logging/logging.h"


namespace Anki {
namespace Cozmo {

  GameMessageHandler::GameMessageHandler()
  : comms_(NULL)
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
  
      Result GameMessageHandler::SendMessage(const UserDeviceID_t devID, const UiMessage& msg)
      {
//#if(RUN_UI_MESSAGE_TCP_SERVER)
        
        Comms::MsgPacket p;
        p.data[0] = msg.GetID();
        msg.GetBytes(p.data+1);
        p.dataLen = msg.GetSize() + 1;
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
        
        const u8 msgID = packet.data[0];
        
        if(lookupTable_[msgID].size != packet.dataLen-1) {
          PRINT_NAMED_ERROR("GameMessageHandler.MessageBufferWrongSize",
                            "Buffer's size does not match expected size for this message ID. (Msg %d, expected %d, recvd %d)\n",
                            msgID,
                            lookupTable_[msgID].size,
                            packet.dataLen - 1
                            );
        }
        else {
        
          RobotID_t robotID = packet.destId;
          
          // This calls the (macro-generated) ProcessPacketAs_MessageX() method
          // indicated by the lookup table, which will cast the buffer as the
          // correct message type and call the specified robot's ProcessMessage(MessageX)
          // method.
          retVal = (*this.*lookupTable_[msgID].ProcessPacketAs)(robotID, packet.data+1);

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
      
      
      Result GameMessageHandler::ProcessMessage(RobotID_t id, MessageG2U_ObjectVisionMarker const& msg)
      {
        // TODO: Do something with this message to notify UI
        printf("RECEIVED OBJECT VISION MARKER: objectID %d\n", msg.objectID);
        
        return RESULT_OK;
      }

  
} // namespace Cozmo
} // namepsace Anki