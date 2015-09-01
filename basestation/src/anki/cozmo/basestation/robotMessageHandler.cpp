/**
 * File: robotMessageHandler.cpp
 *
 * Author: Andrew Stein
 * Date:   1/22/2014
 *
 * Description: Implements the singleton RobotMessageHandler object. See 
 *              corresponding header for more detail.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "util/logging/logging.h"
#include "anki/common/basestation/array2d_impl.h"
#include "anki/vision/CameraSettings.h"
#include "anki/vision/basestation/image.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/cozmo/basestation/robotMessageHandler.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/helpers/includeFstream.h"
#include "util/fileUtils/fileUtils.h"

// Uncomment to allow interprocess access to the camera stream (e.g. Matlab)
//#define STREAM_IMAGES_VIA_FILESYSTEM 1
#if defined(STREAM_IMAGES_VIA_FILESYSTEM) && STREAM_IMAGES_VIA_FILESYSTEM == 1
#include "anki/common/basestation/array2d_impl.h"
#endif

namespace Anki {
  namespace Cozmo {
    
    RobotMessageHandler::RobotMessageHandler(Util::Data::DataPlatform* dataPlatform)
    : _channel(NULL)
    , _robotMgr(NULL)
    , _dataPlatform(dataPlatform)
    {
      
    }

    Result RobotMessageHandler::Init(Comms::IChannel* channel,
                                     RobotManager*  robotMgr)
    {
      Result retVal = RESULT_OK;
      
      //TODO: PRINT_NAMED_DEBUG("RobotMessageHandler", "Initializing comms");
      _channel = channel;
      _robotMgr = robotMgr;
      
      _isInitialized = true;
      
      return retVal;
    }
    

    Result RobotMessageHandler::SendMessage(const RobotID_t robotID, const RobotMessage& msg, bool reliable, bool hot)
    {
      Comms::OutgoingPacket p;
      p.buffer[0] = msg.GetID();
      msg.GetBytes(p.buffer + 1);
      p.bufferSize = msg.GetSize() + 1;
      p.destId = robotID;
      p.reliable = reliable;
      p.hot = hot;
      
      return _channel->Send(p) > 0 ? RESULT_OK : RESULT_FAIL;
    }

    Result RobotMessageHandler::ProcessPacket(const Comms::IncomingPacket& packet)
    {
      if(_robotMgr == NULL) {
        PRINT_STREAM_ERROR("RobotMessageHandler.NullRobotManager",
                          "RobotManager NULL when RobotMessageHandler::ProcessPacket() called.");
        return RESULT_FAIL;
      }
      
      switch (packet.tag) {
        case Comms::IncomingPacket::Tag::Connected:
          if (!_robotMgr->DoesRobotExist(static_cast<RobotID_t>(packet.sourceId))) {
            PRINT_STREAM_ERROR("RobotMessageHandler.Connected",
                               "Incoming connection not found for robot id " << packet.sourceId << ", address " << packet.sourceAddress << ". Disconnecting.");
            return RESULT_FAIL;
          }
          
          PRINT_STREAM_DEBUG("RobotMessageHandler.Connected",
                             "Connection accepted from connection id " << packet.sourceId << ", address " << packet.sourceAddress << ".");
          return RESULT_OK;
        case Comms::IncomingPacket::Tag::Disconnected:
          PRINT_STREAM_INFO("RobotMessageHandler.Disconnected",
                             "Disconnected from connection id " << packet.sourceId << ", address " << packet.sourceAddress << ".");
          _robotMgr->RemoveRobot(static_cast<RobotID_t>(packet.sourceId));
          return RESULT_OK;
        
        case Comms::IncomingPacket::Tag::NormalMessage:
          break;
        
        // should be handled internally by MultiClientChannel, but in case it's a different IChannel
        case Comms::IncomingPacket::Tag::ConnectionRequest:
          _channel->RefuseIncomingConnection(packet.sourceAddress);
          return RESULT_OK;
        
        default:
          return RESULT_FAIL;
      }
      
      if (packet.bufferSize == 0) {
        PRINT_STREAM_ERROR("RobotMessageHandler.ZeroSizeNormalPacket",
                           "Got a normal packet of zero length from " << packet.sourceAddress);
        return RESULT_FAIL;
      }
      
      const u8 msgID = packet.buffer[0];
      
      // Check for invalid msgID
      if (msgID >= RobotMessage::NUM_MSG_IDS || msgID == 0) {
        PRINT_NAMED_ERROR("RobotMessageHandler.InvalidMsgId",
          "Received msgID is invalid (Msg %d, MaxValidID %d)", msgID,RobotMessage::NUM_MSG_IDS);
        return RESULT_FAIL;
      }
      
      // Check that the msg size matches expected size
      if(lookupTable_[msgID].size != packet.bufferSize-1) {
        PRINT_NAMED_ERROR("RobotMessageHandler.MessageBufferWrongSize",
          "Buffer's size does not match expected size for this message ID. (Msg %d, expected %d, recvd %d)",
          msgID, lookupTable_[msgID].size, packet.bufferSize - 1 );
        return RESULT_FAIL;
      }
      
      const RobotID_t robotID = packet.sourceId;
      //Robot* robot = RobotManager::getInstance()->GetRobotByID(robotID);
      Robot* robot = _robotMgr->GetRobotByID(robotID);
      if(robot == NULL) {
        PRINT_NAMED_ERROR("MessageFromInvalidRobotSource", "Message %d received from invalid robot source ID %d.", msgID, robotID);
        return RESULT_FAIL;
      }
      
      if(this->lookupTable_[msgID].ProcessPacketAs == nullptr) {
        PRINT_NAMED_ERROR("RobotMessageHandler.ProcessPacket.NullProcessPacketFcn",
          "Message %d received by robot %d, but no ProcessPacketAs function defined for it.", msgID, robotID);
        return RESULT_FAIL;
      }
      
      // This calls the (macro-generated) ProcessPacketAs_MessageX() method
      // indicated by the lookup table, which will cast the buffer as the
      // correct message type and call the specified robot's ProcessMessage(MessageX)
      // method.
      return (this->*lookupTable_[msgID].ProcessPacketAs)(robot, packet.buffer+1);
    } // ProcessBuffer()
    
    Result RobotMessageHandler::ProcessMessages()
    {
      Result retVal = RESULT_FAIL;
      
      if(_isInitialized) {
        retVal = RESULT_OK;
        
        Comms::IncomingPacket packet;
        while (_channel->PopIncomingPacket(packet)) {
          if(ProcessPacket(packet) != RESULT_OK) {
            retVal = RESULT_FAIL;
          }
        }
      }
      
      return retVal;
    } // ProcessMessages()
    
    
    // TODO: Remove this once we get rid of MessageVisionMarker fully
    // Convert a MessageVisionMarker into a VisionMarker object and hand it off
    // to the BlockWorld
    Result RobotMessageHandler::ProcessMessage(Robot* robot, const MessageVisionMarker& msg)
    {
      Result retVal = RESULT_FAIL;
      
      PRINT_NAMED_ERROR("RobotMessageHandler.ProcessMessage.MessageVisionMarker",
                        "We should not be receiving this message from anywhere.");
      
      return retVal;
    } // ProcessMessage(MessageVisionMarker)
    
    /*
    Result RobotMessageHandler::ProcessMessage(Robot* robot, const MessageFaceDetection& msg)
    {
      Result retVal = RESULT_OK;
      
      // TODO: Do something with face detections
      
      PRINT_INFO("Robot %d reported seeing a face at (x,y,w,h)=(%d,%d,%d,%d).",
                 robot->GetID(), msg.x_upperLeft, msg.y_upperLeft, msg.width, msg.height);
      
      
      //if(msg.visualize > 0) {
        // Send tracker quad info to viz
        const u16 left_x   = msg.x_upperLeft;
        const u16 right_x  = left_x + msg.width;
        const u16 top_y    = msg.y_upperLeft;
        const u16 bottom_y = top_y + msg.height;
        
        VizManager::getInstance()->SendTrackerQuad(left_x, top_y,
                                                   right_x, top_y,
                                                   right_x, bottom_y,
                                                   left_x, bottom_y);
      //}
      
      return retVal;
    } // ProcessMessage(MessageFaceDetection)
    */
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageCameraCalibration const& msg)
    {
      PRINT_NAMED_INFO("RobotMessageHandler.CameraCalibration",
        "Received new %dx%d camera calibration from robot.", msg.ncols, msg.nrows);
      
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
      robot->SetPhysicalRobot(msg.isPhysicalRobot);
      
      return RESULT_OK;
    }
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageRobotState const& msg)
    {
      /*
      PRINT_NAMED_INFO("RobotStateMsgRecvd",
                       "RobotStateMsg received \n"
                       "  ID: %d\n"
                       "  pose (%f,%f,%f,%f), frame %d\n"
                       "  wheel speeds (l=%f, r=%f)\n"
                       "  headAngle %f\n"
                       "  liftAngle %f\n"
                       "  liftHeight %f"
                       ,robot->GetID()
                       ,msg.pose_x, msg.pose_y, msg.pose_z, msg.pose_angle, msg.pose_frame_id
                       ,msg.lwheel_speed_mmps, msg.rwheel_speed_mmps
                       ,msg.headAngle, msg.liftAngle, msg.liftHeight
                       );
       */
      
      return robot->UpdateFullRobotState(msg);
    }

    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessagePrintText const& msg)
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
    /*
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageDockingErrorSignal const& msg)
    {
      VizManager::getInstance()->SetDockingError(msg.x_distErr, msg.y_horErr, msg.angleErr);
      return RESULT_OK;
    }
     */
    

    // For processing image chunks arriving from robot.
    // Sends complete images to VizManager for visualization (and possible saving).
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageImageChunk const& msg)
    {
      const u16 width  = Vision::CameraResInfo[msg.resolution].width;
      const u16 height = Vision::CameraResInfo[msg.resolution].height;

      const bool isImageReady = _imageDeChunker.AppendChunk(msg.imageId, msg.frameTimeStamp,
                                                            height, width,
                                                            (Vision::ImageEncoding_t)msg.imageEncoding,
                                                            msg.imageChunkCount,
                                                            msg.chunkId, msg.data, msg.chunkSize );

      IExternalInterface* externalInterface = robot->GetExternalInterface();
      if (externalInterface != nullptr && robot->GetImageSendMode() != ImageSendMode::Off) {
        ExternalInterface::ImageChunk uiImgChunk;
        uiImgChunk.imageId = msg.imageId;
        uiImgChunk.frameTimeStamp = msg.frameTimeStamp;
        uiImgChunk.nrows = Vision::CameraResInfo[msg.resolution].height;
        uiImgChunk.ncols = Vision::CameraResInfo[msg.resolution].width;
        assert(uiImgChunk.data.size() == msg.data.size());
        uiImgChunk.chunkSize = msg.chunkSize;
        uiImgChunk.imageEncoding = (ImageEncodingClad)msg.imageEncoding;
        uiImgChunk.imageChunkCount = msg.imageChunkCount;
        uiImgChunk.chunkId = msg.chunkId;
        std::copy(msg.data.begin(), msg.data.end(),
          uiImgChunk.data.begin());

        ExternalInterface::MessageEngineToGame msgWrapper;
        msgWrapper.Set_ImageChunk(uiImgChunk);
        robot->GetExternalInterface()->Broadcast(std::move(msgWrapper));

        const bool wasLastChunk = uiImgChunk.chunkId == uiImgChunk.imageChunkCount-1;

        if(wasLastChunk && robot->GetImageSendMode() == ImageSendMode::SingleShot) {
          // We were just in single-image send mode, and the image got sent, so
          // go back to "off". (If in stream mode, stay in stream mode.)
          robot->SetImageSendMode(ImageSendMode::Off);
        }
      }
      VizManager::getInstance()->SendImageChunk(robot->GetID(), msg);
      
      if(isImageReady)
      {
        Vision::Image image;
        cv::Mat cvImg = _imageDeChunker.GetImage();
        if(cvImg.channels() == 1) {
          image = Vision::Image(height, width, cvImg.data);
        } else {
          // TODO: Actually support processing color data (and have ImageRGB object)
          cv::cvtColor(cvImg, cvImg, CV_RGB2GRAY);
          image = Vision::Image(height, width, cvImg.data);
        }
        
        image.SetTimestamp(msg.frameTimeStamp);
        
#       if defined(STREAM_IMAGES_VIA_FILESYSTEM) && STREAM_IMAGES_VIA_FILESYSTEM == 1
        // Create a 50mb ramdisk on OSX at "/Volumes/RamDisk/" by typing: diskutil erasevolume HFS+ 'RamDisk' `hdiutil attach -nomount ram://100000`
        static const char * const g_queueImages_filenamePattern = "/Volumes/RamDisk/robotImage%04d.bmp";
        static const s32 g_queueImages_queueLength = 70; // Must be at least the FPS of the camera. But higher numbers may cause more lag for the consuming process.
        static s32 g_queueImages_queueIndex = 0;
        
        char filename[256];
        snprintf(filename, 256, g_queueImages_filenamePattern, g_queueImages_queueIndex);
        
        cv::imwrite(filename, image.get_CvMat_());
        
        g_queueImages_queueIndex++;
        
        if(g_queueImages_queueIndex >= g_queueImages_queueLength)
          g_queueImages_queueIndex = 0;
#       endif // #if defined(STREAM_IMAGES_VIA_FILESYSTEM) && STREAM_IMAGES_VIA_FILESYSTEM == 1
        
        robot->ProcessImage(image);
        
      } // if(isImageReady)
      
      return RESULT_OK;
    }
    
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageTrackerQuad const& msg)
    {
      /*
      PRINT_INFO("TrackerQuad: (%d,%d) (%d,%d) (%d,%d) (%d,%d)",
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
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageBlockPickedUp const& msg)
    {
      const char* successStr = (msg.didSucceed ? "succeeded" : "failed");
      PRINT_STREAM_INFO("RobotMessageHandler.ProcessMessage.MessageBlockPickedUp", "Robot " << robot->GetID() << " reported it " << successStr << " picking up block. "
                 "Stopping docking and turning on Look-for-Markers mode.");

      Result lastResult = RESULT_OK;
      if(msg.didSucceed) {
        lastResult = robot->SetDockObjectAsAttachedToLift();
        robot->SetLastPickOrPlaceSucceeded(true);
      }
      else {
        robot->SetLastPickOrPlaceSucceeded(false);
      }
      
      // Note: this returns the vision system to whatever mode it was in before
      // it was docking/tracking
      robot->StopDocking();
      
      return lastResult;
    }
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageBlockPlaced const& msg)
    {
      const char* successStr = (msg.didSucceed ? "succeeded" : "failed");
      PRINT_STREAM_INFO("RobotMessageHandler.ProcessMessage.MessageBlockPlaced", "Robot " << robot->GetID() << " reported it " << successStr << " placing block. "
                 "Stopping docking and turning on Look-for-Markers mode.");
      
      Result lastResult = RESULT_OK;
      if(msg.didSucceed) {
        lastResult = robot->SetCarriedObjectAsUnattached();
        robot->SetLastPickOrPlaceSucceeded(true);
      }
      else {
        robot->SetLastPickOrPlaceSucceeded(false);
      }
      
      robot->StopDocking();
      robot->StartLookingForMarkers();
      
      return lastResult;
    }
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageRampTraverseStart const& msg)
    {
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage", "Robot %d reported it started traversing a ramp.", robot->GetID());

      robot->SetOnRamp(true);
      
      return RESULT_OK;
    }
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageRampTraverseComplete const& msg)
    {
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.", "Robot %d reported it completed traversing a ramp.", robot->GetID());

      robot->SetOnRamp(false);
      
      return RESULT_OK;
    }
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageBridgeTraverseStart const& msg)
    {
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.", "Robot %d reported it started traversing a bridge.", robot->GetID());
      
      // TODO: What does this message trigger?
      //robot->SetOnBridge(true);
      
      return RESULT_OK;
    }
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageBridgeTraverseComplete const& msg)
    {
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.", "Robot %d reported it completed traversing a bridge.", robot->GetID());
      
      // TODO: What does this message trigger?
      //robot->SetOnBridge(false);
      
      return RESULT_OK;
    }
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageMainCycleTimeError const& msg)
    {
      Result lastResult = RESULT_OK;
    
      if (msg.numMainTooLongErrors > 0) {
        PRINT_NAMED_WARNING("* * * MainCycleTooLong * * *", "Num errors: %d, Avg time: %d us", msg.numMainTooLongErrors, msg.avgMainTooLongTime);
      }
      
      if (msg.numMainTooLateErrors > 0) {
        PRINT_NAMED_WARNING("* * * MainCycleTooLate * * *", "Num errors: %d, Avg time: %d us", msg.numMainTooLateErrors, msg.avgMainTooLateTime);
      }
      
      return lastResult;
    }

    // For processing imu data chunks arriving from robot.
    // Writes the entire log of 3-axis accelerometer and 3-axis
    // gyro readings to a .m file in kP_IMU_LOGS_DIR so they
    // can be read in from Matlab. (See robot/util/imuLogsTool.m)
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageIMUDataChunk const& msg)
    {
      static u8 imuSeqID = 0;
      static u32 dataSize = 0;
      static s8 imuData[6][1024];  // first ax, ay, az, gx, gy, gz
      
      // If seqID has changed, then start over.
      if (msg.seqId != imuSeqID) {
        imuSeqID = msg.seqId;
        dataSize = 0;
      }
      
      // Msgs are guaranteed to be received in order so just append data to array
      memcpy(imuData[0] + dataSize, msg.aX.data(), IMU_CHUNK_SIZE);
      memcpy(imuData[1] + dataSize, msg.aY.data(), IMU_CHUNK_SIZE);
      memcpy(imuData[2] + dataSize, msg.aZ.data(), IMU_CHUNK_SIZE);
      
      memcpy(imuData[3] + dataSize, msg.gX.data(), IMU_CHUNK_SIZE);
      memcpy(imuData[4] + dataSize, msg.gY.data(), IMU_CHUNK_SIZE);
      memcpy(imuData[5] + dataSize, msg.gZ.data(), IMU_CHUNK_SIZE);
      
      dataSize += IMU_CHUNK_SIZE;
      
      // When dataSize matches the expected size, print to file
      if (msg.chunkId == msg.totalNumChunks - 1) {
        
        // Make sure image capture folder exists
        std::string imuLogsDir = _dataPlatform->pathToResource(Util::Data::Scope::Cache, AnkiUtil::kP_IMU_LOGS_DIR);
        if (!Util::FileUtils::CreateDirectory(imuLogsDir, false, true)) {
          PRINT_NAMED_ERROR("Robot.ProcessIMUDataChunk.CreateDirFailed","%s", imuLogsDir.c_str());
        }
        
        // Create image file
        char logFilename[564];
        snprintf(logFilename, sizeof(logFilename), "%s/robot%d_imu%d.m", imuLogsDir.c_str(), robot->GetID(), imuSeqID);
        PRINT_STREAM_INFO("RobotMessageHandler.ProcessMessage.MessageIMUDataChunk", "Printing imu log to " << logFilename << " (dataSize = " << dataSize << ")");
        
        std::ofstream oFile(logFilename);
        for (u32 axis = 0; axis < 6; ++axis) {
          oFile << "imuData" << axis << " = [";
          for (u32 i=0; i<dataSize; ++i) {
            oFile << (s32)(imuData[axis][i]) << " ";
          }
          oFile << "];\n\n";
        }
        oFile.close();
      }
      
      return RESULT_OK;
    }
    
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageRobotAvailable const&)
    {
      return RESULT_OK;
    }

    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessagePlaySoundOnBaseStation const& msg)
    {
      PRINT_NAMED_ERROR("RobotMessageHandler.ProcessMessage", "MessagePlaySoundOnBaseStation is deprecated!");
      return RESULT_FAIL;
    }
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageStopSoundOnBaseStation const& msg)
    {
      robot->StopSound();
      return RESULT_OK;
    }
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageGoalPose const& msg)
    {
      Anki::Pose3d p(msg.pose_angle, Z_AXIS_3D(),
                     Vec3f(msg.pose_x, msg.pose_y, msg.pose_z));
      //PRINT_INFO("Goal pose: x=%f y=%f %f deg (%d)", msg.pose_x, msg.pose_y, RAD_TO_DEG_F32(msg.pose_angle), msg.followingMarkerNormal);
      if (msg.followingMarkerNormal) {
        VizManager::getInstance()->DrawPreDockPose(100, p, NamedColors::RED);
      } else {
        VizManager::getInstance()->DrawPreDockPose(100, p, NamedColors::GREEN);
      }

      return RESULT_OK;
    }
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageSyncTimeAck const& msg)
    {
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.MessageSyncTimeAck", "SyncTime acknowledged");
      robot->SetSyncTimeAcknowledged(true);

      return RESULT_OK;
    }

    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageBlockIDFlashStarted const& msg)
    {
      printf("TODO: MessageBlockIDFlashStarted at time %d ms\n", msg.timestamp);
      return RESULT_OK;
    }
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageActiveObjectMoved const& msg)
    {
      
      // The message from the robot has the active object ID in it, so we need
      // to find the object in blockworld (which has its own bookkeeping ID) that
      // has the matching active ID
     
      const BlockWorld::ObjectsMapByType_t& activeBlocksByType = robot->GetBlockWorld().GetExistingObjectsByFamily(ObjectFamily::LightCube);
      
      for(auto objectsByID : activeBlocksByType) {
        for(auto objectWithID : objectsByID.second) {
          ObservableObject* object = objectWithID.second;
          assert(object->IsActive());
          if(object->GetActiveID() == msg.objectID) {
            // TODO: Mark object as de-localized
            printf("Received message that Object %d (Active ID %d) moved.\n",
                   objectWithID.first.GetValue(), msg.objectID);
            
            // Don't notify game about moving objects that are being carried
            ActionableObject* actionObject = dynamic_cast<ActionableObject*>(object);
            assert(actionObject != nullptr);
            if(actionObject->IsBeingCarried() == false) {
              robot->GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::ActiveObjectMoved(
                robot->GetID(), objectWithID.first, msg.xAccel, msg.yAccel, msg.zAccel, (UpAxisClad)msg.upAxis
              )));
            }
            
            return RESULT_OK;
          }
        }
      }
      
      printf("Could not find match for active object ID %d\n", msg.objectID);
      return RESULT_FAIL;
    }
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageActiveObjectStoppedMoving const& msg)
    {
      const BlockWorld::ObjectsMapByType_t& activeBlocksByType = robot->GetBlockWorld().GetExistingObjectsByFamily(ObjectFamily::LightCube);
      
      for(auto objectsByID : activeBlocksByType) {
        for(auto objectWithID : objectsByID.second) {
          ObservableObject* object = objectWithID.second;
          assert(object->IsActive());
          if(object->GetActiveID() == msg.objectID) {
            // TODO: Mark object as de-localized
            PRINT_STREAM_INFO("RobotMessageHandler.ProcessMessage."
                              "MessageActiveObjectStoppedMoving",
                              "Received message that Object " << objectWithID.first.GetValue() << " (Active ID " << msg.objectID << ") stopped moving.");
            
            robot->GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::ActiveObjectStoppedMoving(
              robot->GetID(), objectWithID.first, (UpAxisClad)msg.upAxis, msg.rolled
            )));
            return RESULT_OK;
          }
        }
      }
      
      PRINT_STREAM_INFO("RobotMessageHandler.ProcessMessage."
                 "MessageActiveObjectStoppedMoving", "Could not find match for active object ID " << msg.objectID);
      return RESULT_FAIL;
    }
    
    Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageActiveObjectTapped const& msg)
    {
      const BlockWorld::ObjectsMapByType_t& activeBlocksByType = robot->GetBlockWorld().GetExistingObjectsByFamily(ObjectFamily::LightCube);
      
      for(auto objectsByID : activeBlocksByType) {
        for(auto objectWithID : objectsByID.second) {
          ObservableObject* object = objectWithID.second;
          assert(object->IsActive());
          if(object->GetActiveID() == msg.objectID) {

            PRINT_STREAM_INFO("RobotMessageHandler.ProcessMessage."
                              "MessageActiveObjectTapped",
                              "Received message that object " << objectWithID.first.GetValue() << " (Active ID " << msg.objectID << ") was tapped " << (uint32_t)msg.numTaps << " times.");
            
            robot->GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::ActiveObjectTapped(
              robot->GetID(), objectWithID.first, msg.numTaps
            )));

            return RESULT_OK;
          }
        }
      }
      
      PRINT_STREAM_INFO("RobotMessageHandler.ProcessMessage."
                        "MessageActiveObjectTapped", "Could not find match for active object ID " << msg.objectID);
      return RESULT_FAIL;
    }
    
  } // namespace Cozmo
} // namespace Anki
