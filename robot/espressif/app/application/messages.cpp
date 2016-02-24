extern "C" {
#include "client.h"
#include "foregroundTask.h"
}
#include "messages.h"
#include "anki/cozmo/robot/esp.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/common/constantsAndMacros.h"
#include "animationController.h"
#include "rtip.h"
#include "nvStorage.h"
#include "upgradeController.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

namespace Anki {
  namespace Cozmo {
    namespace Messages {

      Result Init()
      {
        return RESULT_OK;
      }
      
      bool taskReadNVAndSend(u32 tag)
      {
        RobotInterface::NVReadResult rslt;
        rslt.blob.tag = tag;
        const NVStorage::NVResult result = NVStorage::Read(rslt.blob);
        if (result == NVStorage::NV_OKAY)
        {
          RobotInterface::SendMessage(rslt);
        }
        else
        {
          NVStorage::NVOpResult msg;
          msg.tag = tag;
          msg.result = result;
          msg.write = false;
          RobotInterface::SendMessage(msg);
        }
        return false;
      }
      
      void ProcessMessage(u8* buffer, u16 bufferSize)
      {
        AnkiConditionalWarnAndReturn(buffer[0] <= RobotInterface::TO_WIFI_END, 1, "Messages", 373, "ToRobot message %x[%d] like like it has tag for engine (> 0x%x)", 3, buffer[0], bufferSize, (int)RobotInterface::TO_WIFI_END);
        if (buffer[0] < RobotInterface::TO_WIFI_START) // Message for someone further down than us
        {
          RTIP::SendMessage(buffer, bufferSize);
        }
        else
        {
          u8 alignedBuffer[260];
          RobotInterface::EngineToRobot* const msg = reinterpret_cast<RobotInterface::EngineToRobot*>(alignedBuffer);
          AnkiConditionalWarnAndReturn(bufferSize <= msg->MAX_SIZE, 1, "Messages", 256, "Received message too big! %02x[%d] > %d", 3, buffer[0], bufferSize, msg->MAX_SIZE);
          switch(buffer[0])
          {
            case RobotInterface::EngineToRobot::Tag_eraseFlash:
            {
              memcpy(msg->GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              UpgradeController::EraseFlash(msg->eraseFlash);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_writeFlash:
            {
              memcpy(msg->GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              UpgradeController::WriteFlash(msg->writeFlash);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_triggerOTAUpgrade:
            {
              memcpy(msg->GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              UpgradeController::Trigger(msg->triggerOTAUpgrade);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_writeNV:
            {
              memcpy(msg->GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              NVStorage::NVOpResult result;
              result.tag    = msg->writeNV.tag;
              result.write  = true;
              result.result = NVStorage::Write(msg->writeNV);
              RobotInterface::SendMessage(result);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_readNV:
            {
              memcpy(msg->GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              switch (msg->readNV.to)
              {
                case NVStorage::ENGINE:
                {
                  foregroundTaskPost(taskReadNVAndSend, msg->readNV.tag);
                  break;
                }
                default:
                {
                  AnkiError( 126, "Messages.readNV", 379, "Reading to target %d not yet supported.", 1, msg->readNV.to)
                }
              }
              break;
            }
            case RobotInterface::EngineToRobot::Tag_rtipVersion:
            {
              memcpy(msg->GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              RTIP::Version = msg->rtipVersion.version;
              RTIP::Date    = msg->rtipVersion.date;
              int i = 0;
              msg->rtipVersion.description_length = MIN(msg->rtipVersion.description_length, VERSION_DESCRIPTION_SIZE-1);
              while (i < msg->rtipVersion.description_length)
              {
                RTIP::VersionDescription[i] = msg->rtipVersion.description[i];
                i++;
              }
              while (i < VERSION_DESCRIPTION_SIZE)
              {
                RTIP::VersionDescription[i] = 0;
                i++;
              }
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
              if(AnimationController::BufferKeyFrame(buffer, bufferSize) != RESULT_OK) {
                AnkiWarn( 1, "Messages", 258, "Failed to buffer a keyframe! Clearing Animation buffer!\n", 0);
                AnimationController::Clear();
              }
              break;
            }
            case RobotInterface::EngineToRobot::Tag_disableAnimTracks:
            {
              memcpy(msg->GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              AnimationController::DisableTracks(msg->disableAnimTracks.whichTracks);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_enableAnimTracks:
            {
              memcpy(msg->GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              AnimationController::EnableTracks(msg->enableAnimTracks.whichTracks);
              break;
            }
            default:
            {
              AnkiWarn( 1, "Messages", 259, "Received message not expected here tag=%02x\n", 1, buffer[0]);
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
