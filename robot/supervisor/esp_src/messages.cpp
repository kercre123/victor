extern "C" {
#include "client.h"
}
#include "messages.h"
#include "animationController.h"
#include "rtip.h"
#include "upgradeController.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "anki/cozmo/robot/esp.h"

namespace Anki {
  namespace Cozmo {
    namespace Messages {

      Result Init()
      {
        return RESULT_OK;
      }
      
      void ProcessMessage(u8* buffer, u16 bufferSize)
      {
        RobotInterface::EngineToRobot msg;
        if (bufferSize > msg.MAX_SIZE)
        {
          PRINT("Received message too big! %02x[%d]\n", buffer[0], bufferSize);
          return;
        }
        else
        {
          memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
          if (!msg.IsValid())
          {
            PRINT("Received invalid message: %02x[%d]\n", buffer[0], bufferSize);
          }
          else if (msg.tag < 0x80) // Message for RTIP not us
          {
            RTIP::SendMessage(msg);
          }
          else
          {
            switch(msg.tag)
            {
              case RobotInterface::EngineToRobot::Tag_eraseFlash:
              {
                UpgradeController::EraseFlash(msg.eraseFlash);
                break;
              }
              case RobotInterface::EngineToRobot::Tag_writeFlash:
              {
                UpgradeController::WriteFlash(msg.writeFlash);
                break;
              }
              case RobotInterface::EngineToRobot::Tag_triggerOTAUpgrade:
              {
                UpgradeController::Trigger(msg.triggerOTAUpgrade);
                break;
              }
              case RobotInterface::EngineToRobot::Tag_abortAnimation:
              {
                AnimationController::Clear();
                break;
              }
              case RobotInterface::EngineToRobot::Tag_animAudioSample:
              case RobotInterface::EngineToRobot::Tag_animAudioSilence:
              case RobotInterface::EngineToRobot::Tag_animHeadAngle:
              case RobotInterface::EngineToRobot::Tag_animLiftHeight:
              case RobotInterface::EngineToRobot::Tag_animFacePosition:
              case RobotInterface::EngineToRobot::Tag_animBlink:
              case RobotInterface::EngineToRobot::Tag_animFaceImage:
              case RobotInterface::EngineToRobot::Tag_animBackpackLights:
              case RobotInterface::EngineToRobot::Tag_animBodyMotion:
              case RobotInterface::EngineToRobot::Tag_animEndOfAnimation:
              case RobotInterface::EngineToRobot::Tag_animStartOfAnimation:
              {
                if(AnimationController::BufferKeyFrame(msg) != RESULT_OK) {
                  //PRINT("Failed to buffer a keyframe! Clearing Animation buffer!\n");
                  AnimationController::Clear();
                }
                break;
              }
              case RobotInterface::EngineToRobot::Tag_disableAnimTracks:
              {
                AnimationController::DisableTracks(msg.disableAnimTracks.whichTracks);
                break;
              }
              case RobotInterface::EngineToRobot::Tag_enableAnimTracks:
              {
                AnimationController::EnableTracks(msg.enableAnimTracks.whichTracks);
                break;
              }
              default:
              {
                PRINT("Received message not expected here tag=%02x\n", msg.tag);
              }
            }
          }
        }
      } 


      int SendText(const char *format, ...)
      {
        va_list argptr;
        va_start(argptr, format);
        SendText(format, argptr);
        va_end(argptr);
        return 0;
      }

      int SendText(const char *format, va_list vaList)
      {
        RobotInterface::PrintText m;
        int len;
                        
        #define MAX_SEND_TEXT_LENGTH 255 // uint_8 definition in messageRobotToEngine.clad
        memset(m.text, 0, MAX_SEND_TEXT_LENGTH);

        // Create formatted text
        len = vsnprintf(m.text, MAX_SEND_TEXT_LENGTH, format, vaList);
        
        if (len > 0)
        {
          m.text_length = len;
          RobotInterface::SendMessage(m);
        }

        return 0;
      }
      
    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki
