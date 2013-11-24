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
    
      // "Private Member Variables"
      namespace {
        // Msg types and their sizes.
        // TODO: Move this into protocol file?
        #define SET_CAMERA_MODE_MSG 0xCA
        #define SIZEOF_SET_CAMERA_MODE_MSG 2
        
        #define BLOCK_POSE_MSG 'E'
        #define SIZEOF_BLOCK_POSE_MSG 13

      }
      
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
          switch(c) {
            case SET_CAMERA_MODE_MSG:
            {
              if (HAL::USBGetNumBytesToRead() < SIZEOF_SET_CAMERA_MODE_MSG)
                return;
              HAL::USBGetChar(); // Clear msg type byte
              
              // Set camera mode
              c = HAL::USBGetChar();
              HAL::SetCameraMode(c);
              PRINT("Change camera res to %d\n", c);
              break;
            }
            case BLOCK_POSE_MSG:
            {
              if (HAL::USBGetNumBytesToRead() < SIZEOF_BLOCK_POSE_MSG)
                return;
              HAL::USBGetChar(); // Clear msg type byte
 
              f32 blockPos_x = FLT_MAX;
              f32 blockPos_y = FLT_MAX;
              f32 blockPos_rad = FLT_MAX;

              if ( !USBGetFloat(blockPos_x) || !USBGetFloat(blockPos_y) || !USBGetFloat(blockPos_rad)) {
                PRINT("ERROR (ProcessUARTMessages): Invalid block pose %f %f %f\n", blockPos_x, blockPos_y, blockPos_rad);
                break;
              }

              // Just testing UART printout
              PERIODIC_PRINT(60, "BlockPose: x = %f, y = %f, rad = %f\n", blockPos_x, blockPos_y, blockPos_rad);
              
              //DockingController::SetRelDockPose(blockPos_x, blockPos_y, blockPos_rad);
              break;
            }
            default:
              PRINT("WARN: (ProcessUARTMsgs): Unexpected char %d\n", c);
              c = HAL::USBGetChar();  // Pop unexpected char from receive buffer
              break;
              
          } // end switch
          
          // Is there another message?
          c = HAL::USBPeekChar();
          
        } // end while
        
      }
      
      
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
          msgSize = msgBuffer[0];
          CozmoMsg_Command cmd = static_cast<CozmoMsg_Command>(msgBuffer[1]);
          switch(cmd)
          {
            case MSG_B2V_CORE_ROBOT_ADDED_TO_WORLD:
            {
              const CozmoMsg_RobotAdded* msg = reinterpret_cast<const CozmoMsg_RobotAdded*>(msgBuffer);
              
              if(msg->robotID != HAL::GetRobotID()) {
                PRINT("Robot received ADDED_TO_WORLD handshake with "
                        " wrong robotID (%d instead of %d).\n",
                        msg->robotID, HAL::GetRobotID());
              }
              
              PRINT("Robot received handshake from basestation, "
                      "sending camera calibration.\n");
              const HAL::CameraInfo *matCamInfo  = HAL::GetMatCamInfo();
              const HAL::CameraInfo *headCamInfo = HAL::GetHeadCamInfo();
              
              CozmoMsg_CameraCalibration calibMsg;
              calibMsg.size = sizeof(CozmoMsg_CameraCalibration);
              
              //
              // Send mat camera calibration
              //
              calibMsg.msgID = MSG_V2B_CORE_MAT_CAMERA_CALIBRATION;
              // TODO: do we send x or y focal length, or both?
              calibMsg.focalLength_x = matCamInfo->focalLength_x;
              calibMsg.focalLength_y = matCamInfo->focalLength_y;
              calibMsg.fov           = matCamInfo->fov_ver;
              calibMsg.nrows         = matCamInfo->nrows;
              calibMsg.ncols         = matCamInfo->ncols;
              calibMsg.center_x      = matCamInfo->center_x;
              calibMsg.center_y      = matCamInfo->center_y;
              
              HAL::RadioToBase(reinterpret_cast<u8*>(&calibMsg),
                               calibMsg.size);
              //
              // Send head camera calibration
              //
              calibMsg.msgID = MSG_V2B_CORE_HEAD_CAMERA_CALIBRATION;
              // TODO: do we send x or y focal length, or both?
              calibMsg.focalLength_x = headCamInfo->focalLength_x;
              calibMsg.focalLength_y = headCamInfo->focalLength_y;
              calibMsg.fov           = headCamInfo->fov_ver;
              calibMsg.nrows         = headCamInfo->nrows;
              calibMsg.ncols         = headCamInfo->ncols;
              calibMsg.center_x      = headCamInfo->center_x;
              calibMsg.center_y      = headCamInfo->center_y;
              
              HAL::RadioToBase(reinterpret_cast<u8*>(&calibMsg),
                               calibMsg.size);
              
              break;
            }
              
            case MSG_B2V_CORE_ABS_LOCALIZATION_UPDATE:
            {
              // TODO: Double-check that size matches expected size?
              
              const CozmoMsg_AbsLocalizationUpdate *msg = reinterpret_cast<const CozmoMsg_AbsLocalizationUpdate*>(msgBuffer);
              
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
              break;
            }
            case MSG_B2V_CORE_INITIATE_DOCK:
            {
              const CozmoMsg_InitiateDock *msg = reinterpret_cast<const CozmoMsg_InitiateDock*>(msgBuffer);
              
              if(DockingController::SetGoals(msg) == EXIT_SUCCESS) {
                
                PRINT("Robot received Initiate Dock message, now in DOCK mode.\n");
                
                Robot::SetOperationMode(Robot::DOCK);
              }
              else {
                PRINT("Initiate Dock message received, failed to set docking goals.\n");
              }
              
              break;
            }

            default:
              PRINT("Unrecognized command in received message.\n");
              
          } // switch(cmd)
          
          
          // Point to next message in buffer
          dataSize -= msgSize;
          msgBuffer = &(dataBuffer[msgSize]);
        }
        
      }
      
      void ProcessIncomingMessages() {
        ProcessBTLEMessages();
        ProcessUARTMessages();
      }
      
    } // namespace CommandHandler
  } // namespace Cozmo
} // namespace Anki
