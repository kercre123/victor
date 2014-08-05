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

#include "anki/common/basestation/utils/logging/logging.h"
#include "anki/vision/CameraSettings.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "messageHandler.h"
#include "vizManager.h"

namespace Anki {
  namespace Cozmo {

#if USE_SINGLETON_MESSAGE_HANDLER
    MessageHandler* MessageHandler::singletonInstance_ = 0;
#endif
    
    MessageHandler::MessageHandler()
    : comms_(NULL), robotMgr_(NULL), blockWorld_(NULL)
    {
      
    }
    Result MessageHandler::Init(Comms::IComms* comms,
                                    RobotManager*  robotMgr,
                                    BlockWorld*    blockWorld)
    {
      Result retVal = RESULT_FAIL;
      
      //TODO: PRINT_NAMED_DEBUG("MessageHandler", "Initializing comms");
      comms_ = comms;
      robotMgr_ = robotMgr;
      blockWorld_ = blockWorld;
      
      if(comms_) {
        isInitialized_ = comms_->IsInitialized();
        if (isInitialized_ == false) {
          // TODO: PRINT_NAMED_ERROR("MessageHandler", "Unable to initialize comms!");
          retVal = RESULT_OK;
        }
      }
      
      return retVal;
    }
    

    Result MessageHandler::SendMessage(const RobotID_t robotID, const Message& msg)
    {
      Comms::MsgPacket p;
      p.data[0] = msg.GetID();
      msg.GetBytes(p.data+1);
      p.dataLen = msg.GetSize() + 1;
      p.destId = robotID;
      
      return comms_->Send(p) > 0 ? RESULT_OK : RESULT_FAIL;
    }

    
    Result MessageHandler::ProcessPacket(const Comms::MsgPacket& packet)
    {
      Result retVal = RESULT_FAIL;
      
      if(robotMgr_ == NULL) {
        PRINT_NAMED_ERROR("MessageHandler.NullRobotManager",
                          "RobotManager NULL when MessageHandler::ProcessPacket() called.\n");
      }
      else {
        const u8 msgID = packet.data[0];
        
        if(lookupTable_[msgID].size != packet.dataLen-1) {
          PRINT_NAMED_ERROR("MessageHandler.MessageBufferWrongSize",
                            "Buffer's size does not match expected size for this message ID. (Msg %d, expected %d, recvd %d)\n",
                            msgID,
                            lookupTable_[msgID].size,
                            packet.dataLen - 1
                            );
        }
        else {
          const RobotID_t robotID = packet.sourceId;
          //Robot* robot = RobotManager::getInstance()->GetRobotByID(robotID);
          Robot* robot = robotMgr_->GetRobotByID(robotID);
          if(robot == NULL) {
            PRINT_NAMED_ERROR("MessageFromInvalidRobotSource",
                              "Message %d received from invalid robot source ID %d.\n",
                              msgID, robotID);
          }
          else {
            // This calls the (macro-generated) ProcessPacketAs_MessageX() method
            // indicated by the lookup table, which will cast the buffer as the
            // correct message type and call the specified robot's ProcessMessage(MessageX)
            // method.
            retVal = (*this.*lookupTable_[msgID].ProcessPacketAs)(robot, packet.data+1);
          }
        }
      } // if(robotMgr_ != NULL)
      
      return retVal;
    } // ProcessBuffer()
    
    Result MessageHandler::ProcessMessages()
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
    
    
    // Convert a MessageVisionMarker into a VisionMarker object and hand it off
    // to the BlockWorld
    Result MessageHandler::ProcessMessage(Robot* robot, const MessageVisionMarker& msg)
    {
      Result retVal = RESULT_FAIL;
      
      if(blockWorld_ == NULL) {
        PRINT_NAMED_ERROR("MessageHandler:NullBlockWorld",
                          "BlockWorld NULL when MessageHandler::ProcessMessage(VisionMarker) called.");
      }
      else {
        CORETECH_ASSERT(robot != NULL);
        retVal = blockWorld_->QueueObservedMarker(msg, *robot);
      }
      
      return retVal;
    } // ProcessMessage(MessageVisionMarker)
    
    
    Result MessageHandler::ProcessMessage(Robot* robot, MessageCameraCalibration const& msg)
    {
      // Convert calibration message into a calibration object to pass to
      // the robot
      Vision::CameraCalibration calib(msg.nrows,
                                      msg.ncols,
                                      msg.focalLength_x,
                                      msg.focalLength_y,
                                      msg.center_x,
                                      msg.center_y,
                                      msg.skew);
      
      robot->SetCameraCalibration(calib);
      
      return RESULT_OK;
    }
    
    Result MessageHandler::ProcessMessage(Robot* robot, MessageRobotState const& msg)
    {
      /*
      PRINT_NAMED_INFO("RobotStateMsgRecvd",
                       "RobotStateMsg received \n"
                       "  ID: %d\n"
                       "  pose (%f,%f,%f,%f), frame %d\n"
                       "  wheel speeds (l=%f, r=%f)\n"
                       "  headAngle %f\n"
                       "  liftAngle %f\n"
                       "  liftHeight %f\n"
                       ,robot->GetID()
                       ,msg.pose_x, msg.pose_y, msg.pose_z, msg.pose_angle, msg.pose_frame_id
                       ,msg.lwheel_speed_mmps, msg.rwheel_speed_mmps
                       ,msg.headAngle, msg.liftAngle, msg.liftHeight
                       );
       */
      
      // Update head angle
      robot->SetHeadAngle(msg.headAngle);

      // Update lift angle
      robot->SetLiftAngle(msg.liftAngle);
      
      // Get ID of last/current path that the robot executed
      robot->SetLastRecvdPathID(msg.lastPathID);
      
      // Update other state vars
      robot->SetCurrPathSegment( msg.currPathSegment );
      robot->SetNumFreeSegmentSlots(msg.numFreeSegmentSlots);
      
      //robot->SetCarryingBlock( msg.status & IS_CARRYING_BLOCK ); // Still needed?
      robot->SetPickingOrPlacing( msg.status & IS_PICKING_OR_PLACING );
      robot->SetPickedUp( msg.status & IS_PICKED_UP );
      
      const f32 WheelSpeedToConsiderStopped = 2.f;
      if(std::abs(msg.lwheel_speed_mmps) < WheelSpeedToConsiderStopped &&
         std::abs(msg.rwheel_speed_mmps) < WheelSpeedToConsiderStopped)
      {
        robot->SetIsMoving(false);
      } else {
        robot->SetIsMoving(true);
      }
      
      // Add to history
      if (robot->AddRawOdomPoseToHistory(msg.timestamp,
                                         msg.pose_frame_id,
                                         msg.pose_x, msg.pose_y, msg.pose_z,
                                         msg.pose_angle,
                                         msg.headAngle,
                                         msg.liftAngle,
                                         robot->GetPoseOrigin()) == RESULT_FAIL) {
        PRINT_NAMED_WARNING("ProcessMessageRobotState.AddPoseError", "t=%d\n", msg.timestamp);
      }
      
      robot->UpdateCurrPoseFromHistory();
      
      return RESULT_OK;
    }

    Result MessageHandler::ProcessMessage(Robot* robot, MessagePrintText const& msg)
    {
      const u32 MAX_PRINT_STRING_LENGTH = 1024;
      static char text[MAX_PRINT_STRING_LENGTH];  // Local storage for large messages which may come across in multiple packets
      static u32 textIdx = 0;

      char *newText = (char*)&(msg.text.front());
      
      // If the last byte is 0, it means this is the last packet (possibly of a series of packets).
      if (msg.text[PRINT_TEXT_MSG_LENGTH-1] == 0) {
        // Text is ready to print
        if (textIdx == 0) {
          // This message is not a part of a longer message. Just print!
          printf("ROBOT-PRINT (%d): %s", robot->GetID(), newText);
        } else {
          // This message is part of a longer message. Copy to local buffer and print.
          memcpy(text + textIdx, newText, strlen(newText)+1);
          printf("ROBOT-PRINT (%d): %s", robot->GetID(), text);
          textIdx = 0;
        }
      } else {
        // This message is part of a larger text. Copy to local buffer. There is more to come!
        memcpy(text + textIdx, newText, PRINT_TEXT_MSG_LENGTH);
        textIdx = MIN(textIdx + PRINT_TEXT_MSG_LENGTH, MAX_PRINT_STRING_LENGTH-1);
        
        // The message received was too long or garbled (i.e. chunks somehow lost)
        if (textIdx == MAX_PRINT_STRING_LENGTH-1) {
          text[MAX_PRINT_STRING_LENGTH-1] = 0;
          printf("ROBOT-PRINT-garbled (%d): %s", robot->GetID(), text);
          textIdx = 0;
        }
        
      }

      return RESULT_OK;
    }
    
    // For visualization of docking error signal
    Result MessageHandler::ProcessMessage(Robot* robot, MessageDockingErrorSignal const& msg)
    {
      VizManager::getInstance()->SetDockingError(msg.x_distErr, msg.y_horErr, msg.angleErr);
      return RESULT_OK;
    }
    

    
    Result MessageHandler::ProcessMessage(Robot* robot, MessageImageChunk const& msg)
    {
      return robot->ProcessImageChunk(msg);
    }
    
    
    Result MessageHandler::ProcessMessage(Robot* robot, MessageTrackerQuad const& msg)
    {
      /*
      PRINT_INFO("TrackerQuad: (%d,%d) (%d,%d) (%d,%d) (%d,%d)\n",
                 msg.topLeft_x, msg.topLeft_y,
                 msg.topRight_x, msg.topRight_y,
                 msg.bottomRight_x, msg.bottomRight_y,
                 msg.bottomLeft_x, msg.bottomRight_y);
      */
     
      // Send tracker quad info to viz
      VizManager::getInstance()->SendTrackerQuad(msg.topLeft_x, msg.topLeft_y,
                                                 msg.topRight_x, msg.topRight_y,
                                                 msg.bottomRight_x, msg.bottomRight_y,
                                                 msg.bottomLeft_x, msg.bottomLeft_y);
      
      return RESULT_OK;
    }
    
    Result MessageHandler::ProcessMessage(Robot* robot, MessageBlockPickedUp const& msg)
    {
      const char* successStr = (msg.didSucceed ? "succeeded" : "failed");
      PRINT_INFO("Robot %d reported it %s picking up block.\n", robot->GetID(), successStr);

      Result lastResult = RESULT_OK;
      if(msg.didSucceed) {
        lastResult = robot->PickUpDockObject();
      }
      else {
        // TODO: what do we do on failure? Need to trigger reattempt?
      }
      
      return lastResult;
    }
    
    Result MessageHandler::ProcessMessage(Robot* robot, MessageBlockPlaced const& msg)
    {
      const char* successStr = (msg.didSucceed ? "succeeded" : "failed");
      PRINT_INFO("Robot %d reported it %s placing block.\n", robot->GetID(), successStr);
      
      Result lastResult = RESULT_OK;
      if(msg.didSucceed) {
        lastResult = robot->PlaceCarriedObject(); //msg.timestamp);
      }
      else {
        // TODO: what do we do on failure? Need to trigger reattempt?
      }
      
      return lastResult;
    }
    
    Result MessageHandler::ProcessMessage(Robot* robot, MessageMainCycleTimeError const& msg)
    {
      Result lastResult = RESULT_OK;
    
      if (msg.numMainTooLongErrors > 0) {
        PRINT_NAMED_WARNING("* * * MainCycleTooLong * * *", "Num errors: %d, Avg time: %d us\n", msg.numMainTooLongErrors, msg.avgMainTooLongTime);
      }
      
      if (msg.numMainTooLateErrors > 0) {
        PRINT_NAMED_WARNING("* * * MainCycleTooLate * * *", "Num errors: %d, Avg time: %d us\n", msg.numMainTooLateErrors, msg.avgMainTooLateTime);
      }
      
      return lastResult;
    }

    

    Result MessageHandler::ProcessMessage(Robot* robot, MessageIMUDataChunk const& msg)
    {
      return robot->ProcessIMUDataChunk(msg);
    }
    
    
    Result MessageHandler::ProcessMessage(Robot* robot, MessageRobotAvailable const&)
    {
      return RESULT_OK;
    }

  } // namespace Cozmo
} // namespace Anki
