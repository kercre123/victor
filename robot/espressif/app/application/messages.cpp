extern "C" {
#include "client.h"
}
#include "messages.h"
#include "anki/cozmo/robot/esp.h"
#include "anki/cozmo/robot/logging.h"
#include "animationController.h"
#include "rtip.h"
#include "upgradeController.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

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
        AnkiConditionalWarnAndReturn(bufferSize <= msg.MAX_SIZE, 1, "Messages", 3, "Received message too big! %02x[%d] > %d", 3, buffer[0], bufferSize, msg.MAX_SIZE);
        memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
        if (msg.tag < 0x80) // Message for RTIP not us
        {
          RTIP::SendMessage(msg);
        }
        else
        {
          AnkiConditionalWarnAndReturn(msg.IsValid(), 1, "Messages", 4, "Received invalid message: %02x[%d]", 2, buffer[0], bufferSize);
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
                AnkiWarn( 1, "Messages", 5, "Failed to buffer a keyframe! Clearing Animation buffer!\n", 0);
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
            case RobotInterface::EngineToRobot::Tag_enterBootloader:
            {
              RTIP::SendMessage(msg);
              break;
            }
            default:
            {
              AnkiWarn( 1, "Messages", 6, "Received message not expected here tag=%02x\n", 1, msg.tag);
            }
          }
        }
      } 


      int SendText(const char *format, ...)
      {
        va_list argptr;
        va_start(argptr, format);
        SendText(RobotInterface::ANKI_LOG_LEVEL_PRINT, format, argptr);
        va_end(argptr);
        return 0;
      }

      int SendText(const RobotInterface::LogLevel level, const char *format, va_list vaList)
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
          m.level = level;
          RobotInterface::SendMessage(m);
        }

        return 0;
      }
      
    } // namespace Messages
    
    namespace RobotInterface {
      int SendLog(const LogLevel level, const uint16_t name, const uint16_t formatId, const uint8_t numArgs, ...)
      {
        static u32 missedMessages = 0;
        PrintTrace m;
        if (missedMessages > 0)
        {
          m.level = ANKI_LOG_LEVEL_WARN;
          m.name  = 1;
          m.stringId = 2;
          m.value_length = 1;
          m.value[0] = missedMessages + 1;
        }
        else
        {
          m.level = level;
          m.name  = name;
          m.stringId = formatId;
          va_list argptr;
          va_start(argptr, numArgs);
          for (m.value_length=0; m.value_length < numArgs; m.value_length++)
          {
            m.value[m.value_length] = va_arg(argptr, int);
          }
          va_end(argptr);
        }
        if (SendMessage(m))
        {
          missedMessages = 0;
        }
        else
        {
          missedMessages++;
        }
        return 0;
      }
    } // namespace RobotInterface
  } // namespace Cozmo
} // namespace Anki
