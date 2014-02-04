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

#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "messageHandler.h"

namespace Anki {
  namespace Cozmo {
    
    MessageHandler* MessageHandler::singletonInstance_ = 0;
    
    ReturnCode MessageHandler::Init(Comms::IComms* comms)
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
    
    ReturnCode MessageHandler::ProcessPacket(const Comms::MsgPacket& packet)
    {
      ReturnCode retVal = EXIT_FAILURE;
      
      const u8 msgID = packet.data[0];
      
      if(lookupTable_[msgID].size != packet.dataLen-1) {
        // TODO: make into "real" error
        fprintf(stderr, "Buffer's size does not match expected size for this message ID.");
        retVal = EXIT_FAILURE;
      } else {
        // This calls the (macro-generated) ProcessPacketAs_MessageX() method
        // indicated by the lookup table, which will cast the buffer as the
        // correct message type and call the specified robot's ProcessMessage(MessageX)
        // method.
        retVal = (*this.*lookupTable_[msgID].ProcessPacketAs)(packet.sourceId, packet.data+1);
      }
      
      return retVal;
    } // ProcessBuffer()
    
    ReturnCode MessageHandler::ProcessMessages()
    {
      
      if(!isInitialized_) {
        return EXIT_FAILURE;
      }
      
      while(comms_->GetNumPendingMsgPackets() > 0)
      {
        Comms::MsgPacket packet;
        comms_->GetNextMsgPacket(packet);
        
        ProcessPacket(packet);
      } // while messages are still available from comms
      
      return EXIT_SUCCESS;
    } // ProcessMessages()
    
    
    // Convert a MessageVisionMarker into a VisionMarker object and hand it off
    // to the BlockWorld
    ReturnCode MessageHandler::ProcessMessage(const RobotID_t robotID, const MessageVisionMarker& msg)
    {
      ReturnCode retVal = EXIT_SUCCESS;
      
      Quad2f corners;
      
      corners[Quad::TopLeft].x()     = msg.get_x_imgUpperLeft();
      corners[Quad::TopLeft].y()     = msg.get_y_imgUpperLeft();
      
      corners[Quad::BottomLeft].x()  = msg.get_x_imgLowerLeft();
      corners[Quad::BottomLeft].y()  = msg.get_y_imgLowerLeft();
      
      corners[Quad::TopRight].x()    = msg.get_x_imgUpperRight();
      corners[Quad::TopRight].y()    = msg.get_y_imgUpperRight();
      
      corners[Quad::BottomRight].x() = msg.get_x_imgLowerRight();
      corners[Quad::BottomRight].y() = msg.get_y_imgLowerRight();
      
      //this->observedVisionMarkers.emplace_back(msg.get_code(), corners, this->ID_);
      const Vision::Camera& camera = RobotManager::getInstance()->GetRobotByID(robotID)->get_camHead();
      Vision::ObservedMarker marker(msg.get_code(), corners, camera);
      
      // Give this vision marker to BlockWorld for processing
      BlockWorld::getInstance()->QueueObservedMarker(marker);
      
      return retVal;
    } // ProcessMessage(MessageVisionMarker)
    
    
    // STUBS:
    ReturnCode MessageHandler::ProcessMessage(const RobotID_t, MessageClearPath const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(const RobotID_t, MessageSetMotion const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(const RobotID_t, MessageRobotState const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(const RobotID_t, MessageRobotAvailable const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(const RobotID_t, MessageMatMarkerObserved const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(const RobotID_t, MessageRobotAddedToWorld const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(const RobotID_t, MessageSetPathSegmentArc const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(const RobotID_t, MessageDockingErrorSignal const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(const RobotID_t, MessageSetPathSegmentLine const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(const RobotID_t, MessageBlockMarkerObserved const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(const RobotID_t, MessageTemplateInitialized const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(const RobotID_t, MessageMatCameraCalibration const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(const RobotID_t, MessageAbsLocalizationUpdate const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(const RobotID_t, MessageHeadCameraCalibration const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(const RobotID_t, MessageTotalVisionMarkersSeen const&){return EXIT_FAILURE;}
    
  } // namespace Cozmo
} // namespace Anki