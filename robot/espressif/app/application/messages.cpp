extern "C" {
#include "client.h"
#include "foregroundTask.h"
// Forward declaration
void ReliableTransport_SetConnectionTimeout(const uint32_t timeoutMicroSeconds);
}
#include "messages.h"
#include "anki/cozmo/robot/esp.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/common/constantsAndMacros.h"
#include "animationController.h"
#include "rtip.h"
#include "face.h"
#include "dhTask.h"
#include "activeObjectManager.h"
#include "factoryTests.h"
#include "nvStorage.h"
#include "wifi_configuration.h"
#include "upgradeController.h"
#include "crashReporter.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

static Anki::Cozmo::NVStorage::NVReportDest nvOpReportTo;

namespace Anki {
  namespace Cozmo {
    namespace Messages {

      Result Init()
      {
        return RESULT_OK;
      }
      
      static void SendNVOpResult(const NVStorage::NVOpResult* report, const NVStorage::NVReportDest dest)
      {
        switch (dest)
        {
          case NVStorage::ENGINE:
          {
            RobotInterface::NVOpResultToEngine msg;
            msg.robotAddress = getSerialNumber();
            os_memcpy(&msg.report, report, sizeof(NVStorage::NVOpResult));
            RobotInterface::SendMessage(msg);
            break;
          }
          case NVStorage::BODY:
          {
            RobotInterface::NVOpResultToBody msg;
            os_memcpy(&msg.report, report, sizeof(NVStorage::NVOpResult));
            RobotInterface::SendMessage(msg);
            break;
          }
          default:
          {
            AnkiError( 151, "Messages.SendNVOpResult", 450, "Unhandled report destination %d", 1, dest);
          }
        }
      }
      
      static void NVWriteDoneCallback(const NVStorage::NVStorageBlob* entry, const NVStorage::NVResult result)
      {
        NVStorage::NVOpResult msg;
        msg.tag    = entry->tag;
        msg.result = result;
        msg.write  = true;
        SendNVOpResult(&msg, nvOpReportTo);
      }
      
      static void NVEraseDoneCallback(const NVStorage::NVEntryTag tag, const NVStorage::NVResult result)
      {
        NVStorage::NVOpResult msg;
        msg.tag = tag;
        msg.result = result;
        msg.write = true;
        SendNVOpResult(&msg, nvOpReportTo);
      }

      static void NVReadDoneCB(NVStorage::NVStorageBlob* entry, const NVStorage::NVResult result)
      {
        if (result != NVStorage::NV_OKAY) // Failed read
        {
          NVStorage::NVOpResult msg;
          msg.tag = ((entry == NULL) ? NVStorage::NVEntry_Invalid : entry->tag);
          msg.result = result;
          msg.write = false;
          SendNVOpResult(&msg, nvOpReportTo);
        }
        else // Successful read, send data
        {
          switch(nvOpReportTo)
          {
            case NVStorage::ENGINE:
            {
              RobotInterface::NVReadResultToEngine msg;
              msg.robotAddress = getSerialNumber();
              os_memcpy(&msg.blob, entry, sizeof(NVStorage::NVStorageBlob));
              RobotInterface::SendMessage(msg);
              break;
            }
            case NVStorage::BODY:
            {
              RobotInterface::NVReadResultToBody msg;
              os_memcpy(&msg.entry, entry, sizeof(NVStorage::NVStorageBlob));
              RobotInterface::SendMessage(msg);
              break;
            }
            default:
            {
              AnkiError( 152, "Messages.NVReadDoneCB", 450, "Unhandled report destination %d", 1, nvOpReportTo);
            }
          }
        }
      }

      static void NVMultiEraseDoneCB(const u32 tag, const NVStorage::NVResult result)
      {
        NVStorage::NVOpResult msg;
        msg.tag    = tag;
        msg.result = result;
        msg.write  = true;
        SendNVOpResult(&msg, nvOpReportTo);
      }
      
      static void NVMultiReadDoneCB(const u32 tag, const NVStorage::NVResult result)
      {
        NVStorage::NVOpResult msg;
        msg.tag    = tag;
        msg.result = result;
        msg.write  = false;
        SendNVOpResult(&msg, nvOpReportTo);
      }
      
      void ProcessMessage(u8* buffer, u16 bufferSize)
      {
        AnkiConditionalWarnAndReturn(buffer[0] <= RobotInterface::TO_WIFI_END, 137, "WiFi.Messages", 394, "ToRobot message %x[%d] like like it has tag for engine (> 0x%x)", 3, buffer[0], bufferSize, (int)RobotInterface::TO_WIFI_END);
        if (buffer[0] < RobotInterface::TO_WIFI_START) // Message for someone further down than us
        {
          RTIP::SendMessage(buffer, bufferSize);
        }
        else
        {
          RobotInterface::EngineToRobot msg;
          AnkiConditionalWarnAndReturn(bufferSize <= msg.MAX_SIZE, 137, "WiFi.Messages", 256, "Received message too big! %02x[%d] > %d", 3, buffer[0], bufferSize, msg.MAX_SIZE);
          switch(buffer[0])
          {
            case RobotInterface::EngineToRobot::Tag_otaWrite:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize);
              UpgradeController::Write(msg.otaWrite);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_writeNV:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              NVStorage::NVOpResult result;
              result.tag    = msg.writeNV.entry.tag;
              result.write  = true;
              if (msg.writeNV.writeNotErase)
              {
                result.result = NVStorage::Write(&msg.writeNV.entry,
                                  (msg.writeNV.reportEach || msg.writeNV.reportDone) ? NVWriteDoneCallback : 0);
              }
              else if (msg.writeNV.rangeEnd == NVStorage::NVEntry_Invalid)
              {
                result.result = NVStorage::Erase(msg.writeNV.entry.tag,
                                  (msg.writeNV.reportEach || msg.writeNV.reportDone) ? NVEraseDoneCallback : 0);
              }
              else
              {
                result.result = NVStorage::EraseRange(msg.writeNV.entry.tag, msg.writeNV.rangeEnd,
                                                      msg.writeNV.reportEach ? NVEraseDoneCallback : 0,
                                                      msg.writeNV.reportDone ? NVMultiEraseDoneCB  : 0);
              }
              if (result.result >= 0) nvOpReportTo = msg.writeNV.reportTo;
              else SendNVOpResult(&result, msg.writeNV.reportTo);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_readNV:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              NVStorage::NVOpResult result;
              result.tag    = msg.readNV.tag;
              result.write  = false;
              if (msg.readNV.tagRangeEnd == NVStorage::NVEntry_Invalid)
              {
                result.result = NVStorage::Read(msg.readNV.tag, NVReadDoneCB);
              }
              else
              {
                result.result = NVStorage::ReadRange(msg.readNV.tag, msg.readNV.tagRangeEnd,
                                                     NVReadDoneCB, NVMultiReadDoneCB);
              }
              if (result.result >= 0) nvOpReportTo = msg.readNV.to;
              else SendNVOpResult(&result, msg.readNV.to);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_wipeAllNV:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              NVStorage::NVOpResult result;
              result.tag = NVStorage::NVEntry_WipeAll;
              result.write = true;
              if (os_strncmp(msg.wipeAllNV.key, "Yes I really want to do this!", msg.wipeAllNV.key_length) != 0)
              {
                result.result = NVStorage::NV_BAD_ARGS;
                SendNVOpResult(&result, msg.wipeAllNV.to);
              }
              else
              {
                result.result = NVStorage::WipeAll(msg.wipeAllNV.doSegments, msg.wipeAllNV.includeFactory, NVEraseDoneCallback, true, msg.wipeAllNV.reboot);
                if (result.result >= 0) nvOpReportTo = msg.wipeAllNV.to;
                else SendNVOpResult(&result, msg.wipeAllNV.to);
              }
              break;
            }
            case RobotInterface::EngineToRobot::Tag_setRTTO:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize);
              AnkiDebug( 144, "ReliableTransport.SetConnectionTimeout", 399, "Timeout is now %dms", 1, msg.setRTTO.timeoutMilliseconds);
              ReliableTransport_SetConnectionTimeout(msg.setRTTO.timeoutMilliseconds * 1000);
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
            case RobotInterface::EngineToRobot::Tag_animEvent:
            case RobotInterface::EngineToRobot::Tag_animFaceImage:
            case RobotInterface::EngineToRobot::Tag_animBackpackLights:
            case RobotInterface::EngineToRobot::Tag_animBodyMotion:
            case RobotInterface::EngineToRobot::Tag_animEndOfAnimation:
            case RobotInterface::EngineToRobot::Tag_animStartOfAnimation:
            {
              if(AnimationController::BufferKeyFrame(buffer, bufferSize) != RESULT_OK) {
                AnkiWarn( 137, "WiFi.Messages", 258, "Failed to buffer a keyframe! Clearing Animation buffer!\n", 0);
                AnimationController::Clear();
              }
              break;
            }
            case RobotInterface::EngineToRobot::Tag_calculateDiffieHellman:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              DiffieHellman::Start(msg.calculateDiffieHellman.local, msg.calculateDiffieHellman.remote);
              break ;
            }
            case RobotInterface::EngineToRobot::Tag_disableAnimTracks:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              AnimationController::DisableTracks(msg.disableAnimTracks.whichTracks);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_enableAnimTracks:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              AnimationController::EnableTracks(msg.enableAnimTracks.whichTracks);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_assignCubeSlots:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              ActiveObjectManager::SetSlots(0, msg.assignCubeSlots.factory_id_length, msg.assignCubeSlots.factory_id);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_oledDisplayNumber:
            {
              using namespace Anki::Cozmo::Face;

              memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              
              u64 frame[COLS];

              Draw::Clear(frame);
              Draw::Number(frame, msg.oledDisplayNumber.digits, msg.oledDisplayNumber.value, msg.oledDisplayNumber.x, msg.oledDisplayNumber.y);
              Draw::Flip(frame);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_testState:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              Factory::Process_TestState(msg.testState);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_enterTestMode:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              Factory::Process_EnterFactoryTestMode(msg.enterTestMode);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_appConCfgString:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              WiFiConfiguration::ProcessConfigString(msg.appConCfgString);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_appConCfgFlags:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              WiFiConfiguration::ProcessConfigFlags(msg.appConCfgFlags);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_appConCfgIPInfo:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              WiFiConfiguration::ProcessConfigIPInfo(msg.appConCfgIPInfo);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_appConGetRobotIP:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              WiFiConfiguration::SendRobotIpInfo(msg.appConGetRobotIP.ifId);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_wifiOff:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              WiFiConfiguration::Off(msg.wifiOff.sleep);
              break;
            }
            case RobotInterface::EngineToRobot::Tag_bodyStorageContents:
            {
              memcpy(msg.GetBuffer(), buffer, bufferSize); // Copy out into aligned struct
              CrashReporter::AcceptBodyStorage(msg.bodyStorageContents);
              break;
            }
            default:
            {
              AnkiWarn( 137, "WiFi.Messages", 259, "Received message not expected here tag=%02x", 1, buffer[0]);
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
