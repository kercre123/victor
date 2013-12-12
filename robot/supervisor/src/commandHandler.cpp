#include "anki/cozmo/messageProtocol.h"

#include "anki/cozmo/robot/commandHandler.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/cozmoTypes.h"
#include "dockingController.h"
#include "anki/cozmo/robot/hal.h" // simulated or real!
#include "anki/cozmo/robot/localization.h"
#include "anki/cozmo/robot/pathFollower.h"
#include "anki/messaging/robot/utilMessaging.h"


namespace Anki {
  namespace Cozmo {
    namespace CommandHandler {
      
      bool USBGetFloat(f32 &val)
      {
        s32 c;
        u32 res = 0;
        for (u8 i=0; i<4; ++i) {
          c = HAL::USBGetChar();
          if (c < 0)
            return false;
          
          res = (res << 8) | c;
        }

        val = *(f32*)(&res);
        return true;
      }
      
      void ProcessUARTMessages()
      {
        s32 c = HAL::USBPeekChar();
        
        while (c >= 0) {
          switch(static_cast<u8>(c)) {
            default:
              PRINT("WARN: (ProcessUARTMsgs): Unexpected char %d\n", c);
              c = HAL::USBGetChar();  // Pop unexpected char from receive buffer
              break;
              
          } // end switch
          
          // Is there another message?
          c = HAL::USBPeekChar();
          
        } // end while
        
      }
      
      
      CozmoMessageID ProcessMessage(const u8* buffer)
      {
        CozmoMessageID msgID = static_cast<CozmoMessageID>(buffer[0]);
        
        (*MessageTable[msgID].dispatchFcn)(buffer+1);
        
        PRINT("ProcessMessage(): Dispatching message with ID=%d.\n", msgID);
        
        return msgID;
      }
      
      void ProcessRobotAddedToWorldMessage(const u8* buffer)
      {
        const CozmoMsg_RobotAddedToWorld* msg = reinterpret_cast<const CozmoMsg_RobotAddedToWorld*>(buffer);
        
        if(msg->robotID != HAL::GetRobotID()) {
          PRINT("Robot received ADDED_TO_WORLD handshake with "
                " wrong robotID (%d instead of %d).\n",
                msg->robotID, HAL::GetRobotID());
        }
        
        PRINT("Robot received handshake from basestation, "
              "sending camera calibration.\n");
        const HAL::CameraInfo *matCamInfo  = HAL::GetMatCamInfo();
        const HAL::CameraInfo *headCamInfo = HAL::GetHeadCamInfo();

        //
        // Send mat camera calibration
        //
        //   TODO: do we send x or y focal length, or both?
        CozmoMsg_MatCameraCalibration matCalibMsg = {
          matCamInfo->focalLength_x,
          matCamInfo->focalLength_y,
          matCamInfo->fov_ver,
          matCamInfo->center_x,
          matCamInfo->center_y,
          matCamInfo->skew,
          matCamInfo->nrows,
          matCamInfo->ncols};
        
        HAL::RadioToBase(&matCalibMsg, GET_MESSAGE_ID(MatCameraCalibration));
        
        //
        // Send head camera calibration
        //
        //   TODO: do we send x or y focal length, or both?
        CozmoMsg_HeadCameraCalibration headCalibMsg = {
          headCamInfo->focalLength_x,
          headCamInfo->focalLength_y,
          headCamInfo->fov_ver,
          headCamInfo->center_x,
          headCamInfo->center_y,
          headCamInfo->skew,
          headCamInfo->nrows,
          headCamInfo->ncols};
          
        HAL::RadioToBase(&headCalibMsg, GET_MESSAGE_ID(HeadCameraCalibration));
        
      } // ProcessRobotAddedMessage()
      
      
      void ProcessAbsLocalizationUpdateMessage(const u8* buffer)
      {
        // TODO: Double-check that size matches expected size?
        
        const CozmoMsg_AbsLocalizationUpdate *msg = reinterpret_cast<const CozmoMsg_AbsLocalizationUpdate*>(buffer);
        
        f32 currentMatX       = msg->xPosition * .001f; // store in meters
        f32 currentMatY       = msg->yPosition * .001f; //     "
        Radians currentMatHeading = msg->headingAngle;
        Localization::SetCurrentMatPose(currentMatX, currentMatY, currentMatHeading);
        
        PRINT("Robot received localization update from "
              "basestation: (%.3f,%.3f) at %.1f degrees\n",
              currentMatX, currentMatY,
              currentMatHeading.getDegrees());
#if(USE_OVERLAY_DISPLAY)
        {
          using namespace Sim::OverlayDisplay;
          SetText(CURR_POSE, "Pose: (x,y)=(%.4f,%.4f) at angle=%.1f\n",
                  currentMatX, currentMatY,
                  currentMatHeading.getDegrees());
        }
#endif

      } // ProcessAbsLocalizationUpdateMessage()
      
      void ProcessBTLEMessages()
      {
        // Process any messages from the basestation
        u8 dataBuffer[HAL::RADIO_BUFFER_SIZE];
        u8* msgBuffer = dataBuffer;
        u8 dataSize, msgSize;
        
        // Get all received data
        dataSize = HAL::RadioFromBase(dataBuffer);
        
        while(dataSize > 0)
        {
          CozmoMessageID msgID = ProcessMessage(msgBuffer);
          
          // Point to next message in buffer
          msgSize = MessageTable[msgID].size;
          dataSize -= msgSize;
          msgBuffer = &(dataBuffer[msgSize]);
        }
        
      } // ProcessBTLEMessages()
      
      void ProcessIncomingMessages() {
        ProcessBTLEMessages();
#if !USE_OFFBOARD_VISION
        // If using offboard vision, that will handle processing UART messages
        ProcessUARTMessages();
#endif
      }
      
      // TODO: Fill these in once they are needed/used:
      void ProcessClearPathMessage(const u8* buffer) {
        PRINT("%s not yet implemented!\n", __PRETTY_FUNCTION__);
      }
      
      void ProcessSetMotionMessage(const u8* buffer) {
        PRINT("%s not yet implemented!\n", __PRETTY_FUNCTION__);
      }

      void ProcessRobotAvailableMessage(const u8* buffer) {
        PRINT("%s not yet implemented!\n", __PRETTY_FUNCTION__);
      }
      
      void ProcessMatMarkerObservedMessage(const u8* buffer) {
        PRINT("%s not yet implemented!\n", __PRETTY_FUNCTION__);
      }
      
      void ProcessSetPathSegmentArcMessage(const u8* buffer) {
        PRINT("%s not yet implemented!\n", __PRETTY_FUNCTION__);
      }
      
      void ProcessSetPathSegmentLineMessage(const u8* buffer) {
        PRINT("%s not yet implemented!\n", __PRETTY_FUNCTION__);
      }
      
      // These need implementations to avoid linker errors, but we don't expect
      // to _receive_ these message types, only to send them.
      void ProcessMatCameraCalibrationMessage(const u8* buffer) {
        PRINT("%s called unexpectedly on the Robot.\n", __PRETTY_FUNCTION__);
      }

      void ProcessHeadCameraCalibrationMessage(const u8* buffer) {
        PRINT("%s called unexpectedly on the Robot.\n", __PRETTY_FUNCTION__);
      }

      
    } // namespace CommandHandler
  } // namespace Cozmo
} // namespace Anki
