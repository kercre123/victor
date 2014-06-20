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

#include "anki/vision/CameraSettings.h"
#include "anki/vision/basestation/imageIO.h"
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
        Vision::Camera camera(robot->GetCamera());
        
        if(camera.IsCalibrated()) {
          
          // Get corners
          Quad2f corners;
          
          corners[Quad::TopLeft].x()     = msg.x_imgUpperLeft;
          corners[Quad::TopLeft].y()     = msg.y_imgUpperLeft;
          
          corners[Quad::BottomLeft].x()  = msg.x_imgLowerLeft;
          corners[Quad::BottomLeft].y()  = msg.y_imgLowerLeft;
          
          corners[Quad::TopRight].x()    = msg.x_imgUpperRight;
          corners[Quad::TopRight].y()    = msg.y_imgUpperRight;
          
          corners[Quad::BottomRight].x() = msg.x_imgLowerRight;
          corners[Quad::BottomRight].y() = msg.y_imgLowerRight;
          
          
          // Get historical robot pose at specified timestamp to get
          // head angle and to attach as parent of the camera pose.
          TimeStamp_t t;
          RobotPoseStamp* p = nullptr;
          HistPoseKey poseKey;
          if (robot->ComputeAndInsertPoseIntoHistory(msg.timestamp, t, &p, &poseKey) == RESULT_FAIL) {
            PRINT_NAMED_WARNING("MessageHandler.ProcessMessageVisionMarker.HistoricalPoseNotFound", "Time: %d, hist: %d to %d\n", msg.timestamp, robot->GetPoseHistory().GetOldestTimeStamp(), robot->GetPoseHistory().GetNewestTimeStamp());
            return RESULT_FAIL;
          }
          
          // Compute pose from robot body to camera
          // Start with canonical (untilted) headPose
          Pose3d camPose(robot->GetHeadCamPose());

          // Rotate that by the given angle
          RotationVector3d Rvec(-p->GetHeadAngle(), Y_AXIS_3D);
          camPose.rotateBy(Rvec);
          
          // Precompute with robot body to neck pose
          camPose.preComposeWith(robot->GetNeckPose());
          
          // Set parent pose to be the historical robot pose
          camPose.set_parent(&(p->GetPose()));
          
          // Update the head camera's pose
          camera.SetPose(camPose);

          // Create observed marker
          Vision::ObservedMarker marker(t, msg.markerType, corners, camera);
          
          // Give this vision marker to BlockWorld for processing
          blockWorld_->QueueObservedMarker(marker, poseKey);
        }
        else {
          PRINT_NAMED_WARNING("MessageHandler::CalibrationNotSet",
                              "Received VisionMarker message from robot before "
                              "camera calibration was set on Basestation.");
        }
        retVal = RESULT_OK;
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
                       "  pose (%f,%f,%f,%f)\n"
                       "  wheel speeds (l=%f, r=%f)\n"
                       "  headAngle %f\n"
                       "  liftAngle %f\n"
                       "  liftHeight %f\n",
                       robot->get_ID(),
                       msg.pose_x, msg.pose_y, msg.pose_z, msg.pose_angle,
                       msg.lwheel_speed_mmps, msg.rwheel_speed_mmps,
                       msg.headAngle, msg.liftAngle, msg.liftHeight);
      */
      
      // Update head angle
      robot->SetHeadAngle(msg.headAngle);

      // Update lift angle
      robot->SetLiftAngle(msg.liftAngle);
      
      /*
      // Update robot pose
      Vec3f axis(0,0,1);
      Vec3f translation(msg.pose_x, msg.pose_y, msg.pose_z);
      robot->set_pose(Pose3d(msg.pose_angle, axis, translation));
      */
      // Get ID of last/current path that the robot executed
      robot->SetLastRecvdPathID(msg.lastPathID);
      
      // Update other state vars
      robot->SetCurrPathSegment( msg.currPathSegment );
      robot->SetNumFreeSegmentSlots(msg.numFreeSegmentSlots);
      
      //robot->SetCarryingBlock( msg.status & IS_CARRYING_BLOCK ); // Still needed?
      robot->SetPickingOrPlacing( msg.status & IS_PICKING_OR_PLACING );
      
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
                                         msg.liftAngle) == RESULT_FAIL) {
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
      static u8 imgID = 0;
      static u32 totalImgSize = 0;
      static u8 data[ 320*240 ];
      static u32 dataSize = 0;
      static u32 width;
      static u32 height;

      //PRINT_INFO("Img %d, chunk %d, size %d, res %d, dataSize %d\n",
      //           msg.imageId, msg.chunkId, msg.chunkSize, msg.resolution, dataSize);
      
      // Check that resolution is supported
      if (msg.resolution != Vision::CAMERA_RES_QVGA &&
          msg.resolution != Vision::CAMERA_RES_QQVGA &&
          msg.resolution != Vision::CAMERA_RES_QQQVGA &&
          msg.resolution != Vision::CAMERA_RES_QQQQVGA) {
        return RESULT_FAIL;
      }
      
      // If msgID has changed, then start over.
      if (msg.imageId != imgID) {
        imgID = msg.imageId;
        dataSize = 0;
        width = Vision::CameraResInfo[msg.resolution].width;
        height = Vision::CameraResInfo[msg.resolution].height;
        totalImgSize = width * height;
      }
      
      // Msgs are guaranteed to be received in order so just append data to array
      memcpy(data + dataSize, msg.data.data(), msg.chunkSize);
      dataSize += msg.chunkSize;
        
      // When dataSize matches the expected size, print to file
      if (dataSize >= totalImgSize) {
#if(0)
        char imgCaptureFilename[64];
        snprintf(imgCaptureFilename, sizeof(imgCaptureFilename), "robot%d_img%d.pgm", robot->get_ID(), imgID);
        PRINT_INFO("Printing image to %s\n", imgCaptureFilename);
        Vision::WritePGM(imgCaptureFilename, data, width, height);
#endif
        VizManager::getInstance()->SendGreyImage(data, (Vision::CameraResolution)msg.resolution);
      }
      
      
      return RESULT_OK;
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
      PRINT_INFO("Robot %d %s picking up block.\n", robot->GetID(), successStr);

      Result lastResult = RESULT_OK;
      if(msg.didSucceed) {
        lastResult = robot->PickUpDockBlock();
      }
      else {
        // TODO: what do we do on failure? Need to trigger reattempt?
      }
      
      return lastResult;
    }
    
    Result MessageHandler::ProcessMessage(Robot* robot, MessageBlockPlaced const& msg)
    {
      Result lastResult = RESULT_OK;
      if(msg.didSucceed) {
        lastResult = robot->PlaceCarriedBlock(); //msg.timestamp);
      }
      else {
        PRINT_INFO("Robot %d FAILED placing block.\n", robot->GetID());
        // TODO: what do we do on failure? Need to trigger reattempt?
      }
      
      return lastResult;
    }
    
    
    // STUBS:
    Result MessageHandler::ProcessMessage(Robot* robot, MessageClearPath const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageDriveWheels const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageDriveWheelsCurvature const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageMoveLift const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageMoveHead const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageSetLiftHeight const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageSetHeadAngle const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageStopAllMotors const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageRobotAvailable const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageRobotInit const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageAppendPathSegmentArc const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageAppendPathSegmentPointTurn const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageTrimPath const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageExecutePath const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageDockWithBlock const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessagePlaceBlockOnGround const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageAppendPathSegmentLine const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageAbsLocalizationUpdate const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageHeadAngleUpdate const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageImageRequest const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageStartTestMode const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageSetHeadlight const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageSetHeadControllerGains const&){return RESULT_FAIL;}
    Result MessageHandler::ProcessMessage(Robot* robot, MessageSetLiftControllerGains const&){return RESULT_FAIL;}
    
  } // namespace Cozmo
} // namespace Anki