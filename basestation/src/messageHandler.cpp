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

#if USE_SINGLETON_MESSAGE_HANDLER
    MessageHandler* MessageHandler::singletonInstance_ = 0;
#endif
    
    MessageHandler::MessageHandler()
    : comms_(NULL), robotMgr_(NULL), blockWorld_(NULL)
    {
      
    }
    ReturnCode MessageHandler::Init(Comms::IComms* comms,
                                    RobotManager*  robotMgr,
                                    BlockWorld*    blockWorld)
    {
      ReturnCode retVal = EXIT_FAILURE;
      
      //TODO: PRINT_NAMED_DEBUG("MessageHandler", "Initializing comms");
      comms_ = comms;
      robotMgr_ = robotMgr;
      blockWorld_ = blockWorld;
      
      if(comms_) {
        isInitialized_ = comms_->IsInitialized();
        if (isInitialized_ == false) {
          // TODO: PRINT_NAMED_ERROR("MessageHandler", "Unable to initialize comms!");
          retVal = EXIT_SUCCESS;
        }
      }
      
      return retVal;
    }
    

    ReturnCode MessageHandler::SendMessage(const RobotID_t robotID, const Message& msg)
    {
      Comms::MsgPacket p;
      p.data[0] = msg.GetID();
      msg.GetBytes(p.data+1);
      p.dataLen = msg.GetSize() + 1;
      p.destId = robotID;
      
      return comms_->Send(p) > 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    
    ReturnCode MessageHandler::ProcessPacket(const Comms::MsgPacket& packet)
    {
      ReturnCode retVal = EXIT_FAILURE;
      
      if(robotMgr_ == NULL) {
        PRINT_NAMED_ERROR("MessageHandler.NullRobotManager",
                          "RobotManager NULL when MessageHandler::ProcessPacket() called.");
      }
      else {
        const u8 msgID = packet.data[0];
        
        if(lookupTable_[msgID].size != packet.dataLen-1) {
          PRINT_NAMED_ERROR("MessageHandler.MessageBufferWrongSize",
                            "Buffer's size does not match expected size for this message ID. (Msg %d, expected %d, recvd %d)",
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
                              "Message %d received from invalid robot source ID %d.",
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
    
    ReturnCode MessageHandler::ProcessMessages()
    {
      ReturnCode retVal = EXIT_FAILURE;
      
      if(isInitialized_) {
        retVal = EXIT_SUCCESS;
        
        while(comms_->GetNumPendingMsgPackets() > 0)
        {
          Comms::MsgPacket packet;
          comms_->GetNextMsgPacket(packet);
          
          if(ProcessPacket(packet) != EXIT_SUCCESS) {
            retVal = EXIT_FAILURE;
          }
        } // while messages are still available from comms
      }
      
      return retVal;
    } // ProcessMessages()
    
    
    // Convert a MessageVisionMarker into a VisionMarker object and hand it off
    // to the BlockWorld
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, const MessageVisionMarker& msg)
    {
      ReturnCode retVal = EXIT_FAILURE;
      
      if(blockWorld_ == NULL) {
        PRINT_NAMED_ERROR("MessageHandler:NullBlockWorld",
                          "BlockWorld NULL when MessageHandler::ProcessMessage(VisionMarker) called.");
      }
      else {
        Quad2f corners;
        
        corners[Quad::TopLeft].x()     = msg.x_imgUpperLeft;
        corners[Quad::TopLeft].y()     = msg.y_imgUpperLeft;
        
        corners[Quad::BottomLeft].x()  = msg.x_imgLowerLeft;
        corners[Quad::BottomLeft].y()  = msg.y_imgLowerLeft;
        
        corners[Quad::TopRight].x()    = msg.x_imgUpperRight;
        corners[Quad::TopRight].y()    = msg.y_imgUpperRight;
        
        corners[Quad::BottomRight].x() = msg.x_imgLowerRight;
        corners[Quad::BottomRight].y() = msg.y_imgLowerRight;
        
        CORETECH_ASSERT(robot != NULL);
        
        const Vision::Camera& camera = robot->get_camHead();
        Vision::ObservedMarker marker(msg.markerType, corners, camera);
        
        // Give this vision marker to BlockWorld for processing
        blockWorld_->QueueObservedMarker(marker);
        
        retVal = EXIT_SUCCESS;
      }
      
      return retVal;
    } // ProcessMessage(MessageVisionMarker)
    
    
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageHeadCameraCalibration const& msg)
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
      
      robot->set_camCalibration(calib);
      
      return EXIT_SUCCESS;
    }
    
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageRobotState const& msg)
    {
      /*
      PRINT_NAMED_INFO("RobotStateMsgRecvd",
                       "RobotStateMsg received \n"
                       "  ID: %d\n"
                       "  pose (%f,%f,%f,%f)\n"
                       "  wheel speeds (l=%f, r=%f)\n"
                       "  headAngle %f\n"
                       "  liftHeight %f\n",
                       robot->get_ID(),
                       msg.pose_x, msg.pose_y, msg.pose_z, msg.pose_angle,
                       msg.lwheel_speed_mmps, msg.rwheel_speed_mmps,
                       msg.headAngle, msg.liftHeight);
      */
      
      // Update head angle
      robot->set_headAngle(msg.headAngle);

      
      // Update robot pose
      Vec3f axis(0,0,1);
      Vec3f translation(msg.pose_x, msg.pose_y, msg.pose_z);
      robot->set_pose(Pose3d(msg.pose_angle, axis, translation));
      
      // Update other state vars
      robot->SetTraversingPath( msg.isTraversingPath );
      robot->SetCarryingBlock( msg.isCarryingBlock );
      
      return EXIT_SUCCESS;
    }

    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessagePrintText const& msg)
    {
      static char text[512];  // Local storage for large messages which may come across in multiple packets
      static u32 textIdx = 0;

      char *newText = (char*)&(msg.text.front());
      
      // If the last byte is 0, it means this is the last packet (possibly of a series of packets).
      if (msg.text[PRINT_TEXT_MSG_LENGTH-1] == 0) {
        // Text is ready to print
        if (textIdx == 0) {
          // This message is not a part of a longer message. Just print!
          printf("ROBOT-PRINT: %s", newText);
        } else {
          // This message is part of a longer message. Copy to local buffer and print.
          memcpy(text + textIdx, newText, strlen(newText)+1);
          printf("ROBOT-PRINT: %s", text);
          textIdx = 0;
        }
      } else {
        // This message is part of a larger text. Copy to local buffer. There is more to come!
        memcpy(text + textIdx, newText, PRINT_TEXT_MSG_LENGTH);
        textIdx += PRINT_TEXT_MSG_LENGTH;
      }

      return EXIT_SUCCESS;
    }
    
    
    // STUBS:
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageClearPath const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageDriveWheels const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageDriveWheelsCurvature const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageMoveLift const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageMoveHead const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageStopAllMotors const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageRobotAvailable const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageMatMarkerObserved const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageRobotAddedToWorld const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageAppendPathSegmentArc const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageExecutePath const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageDockWithBlock const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageDockingErrorSignal const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageAppendPathSegmentLine const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageBlockMarkerObserved const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageMatCameraCalibration const&){return EXIT_FAILURE;}
    ReturnCode MessageHandler::ProcessMessage(Robot* robot, MessageAbsLocalizationUpdate const&){return EXIT_FAILURE;}
    
  } // namespace Cozmo
} // namespace Anki