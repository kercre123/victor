/**
 * File: messageHandler.cpp
 *
 * Author: Andrew Stein
 * Date:   1/22/2014
 *
 * Description: Implements the singleton MessageHandler object. See 
 *              corresponding header for more detail.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "messageHandler.h"

namespace Anki {
  namespace Cozmo {
    
    MessageHandler* MessageHandler::singletonInstance_ = 0;
    
    ReturnCode MessageHandler::Init(Comms::CommsManager* comms)
    {
      ReturnCode retVal = EXIT_FAILURE;
      
      //TODO: PRINT_NAMED_DEBUG("MessageHandler", "Initializing comms");
      comms_ = comms;
      
      if(comms_) {
        isInitialized_ = comms_->IsInitialized();
        if (isInitialized_ == false) {
          // TODO: PRINT_NAMED_ERROR("MessageHandler", "Unable to initialize comms!");
          retVal = EXIT_SUCCESS;
        }
      }
      
      return retVal;
    }
    
    MessageHandler::MessageHandler()
    {
      robotMgr_ = RobotManager::getInstance();
    }
    
    ReturnCode MessageHandler::ProcessBuffer(const RobotID_t robotID, const u8* buffer, const u8 bufferSize)
    {
      ReturnCode retVal = EXIT_FAILURE;
      
      const u8 msgID = buffer[0];
      
      if(lookupTable_[msgID].size != bufferSize-1) {
        // TODO: make into "real" error
        fprintf(stderr, "Buffer's size does not match expected size for this message ID.");
        retVal = EXIT_FAILURE;
      } else {
        // This calls the (macro-generated) ProcessBufferAs_MessageX() method
        // indicated by the lookup table, which will cast the buffer as the
        // correct message type and call the specified robot's ProcessMessage(MessageX)
        // method.
        retVal = (*this.*lookupTable_[msgID].ProcessBufferAs)(robotID, buffer+1);
      }
      
      return retVal;
    }
    
  } // namespace Cozmo
} // namespace Anki