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
#include "factoryTests.h"
#include "nvStorage.h"
#include "wifi_configuration.h"
#include "upgradeController.h"
#include "crashReporter.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

namespace Anki {
  namespace Cozmo {
    namespace Messages {

      Result Init()
      {
        return RESULT_OK;
      }
      
      static void NVOpCallback(NVStorage::NVOpResult& msg)
      {
        RobotInterface::SendMessage(msg);
      }
      
      void ProcessMessage(RobotInterface::EngineToRobot& msg)
      {
        switch(msg.tag)
        {
          case RobotInterface::EngineToRobot::Tag_otaWrite:
          {
            UpgradeController::Write(msg.otaWrite);
            break;
          }
          case RobotInterface::EngineToRobot::Tag_commandNV:
          {
            const NVStorage::NVResult rslt = NVStorage::Command(msg.commandNV, NVOpCallback);
            if (rslt != NVStorage::NV_SCHEDULED)
            {
              NVStorage::NVOpResult resp;
              resp.address = msg.commandNV.address;
              resp.offset  = 0;
              resp.operation = msg.commandNV.operation;
              resp.result = rslt;
              resp.blob_length = 0;
              RobotInterface::SendMessage(resp);
            }
            break;
          }
          case RobotInterface::EngineToRobot::Tag_setRTTO:
          {
            AnkiDebug( 144, "ReliableTransport.SetConnectionTimeout", 399, "Timeout is now %dms", 1, msg.setRTTO.timeoutMilliseconds);
            ReliableTransport_SetConnectionTimeout(msg.setRTTO.timeoutMilliseconds * 1000);
            break;
          }
          case RobotInterface::EngineToRobot::Tag_abortAnimation:
          {
            AnimationController::Clear();
            break;
          }
          case RobotInterface::EngineToRobot::Tag_calculateDiffieHellman:
          {
            DiffieHellman::Start(msg.calculateDiffieHellman.local, msg.calculateDiffieHellman.remote);
            break ;
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
          case RobotInterface::EngineToRobot::Tag_initAnimController:
          {
            AnimationController::EngineInit(msg.initAnimController);
            break;
          }
          case RobotInterface::EngineToRobot::Tag_oledDisplayNumber:
          {
            using namespace Anki::Cozmo::Face;
            u64 frame[COLS];
            Draw::Clear(frame);
            Draw::Number(frame, msg.oledDisplayNumber.digits, msg.oledDisplayNumber.value, msg.oledDisplayNumber.x, msg.oledDisplayNumber.y);
            Draw::Flip(frame);
            break;
          }
          case RobotInterface::EngineToRobot::Tag_testState:
          {
            Factory::Process_TestState(msg.testState);
            break;
          }
          case RobotInterface::EngineToRobot::Tag_enterTestMode:
          {
            Factory::Process_EnterFactoryTestMode(msg.enterTestMode);
            break;
          }
          case RobotInterface::EngineToRobot::Tag_appConCfgString:
          {
            WiFiConfiguration::ProcessConfigString(msg.appConCfgString);
            break;
          }
          case RobotInterface::EngineToRobot::Tag_appConCfgFlags:
          {
            WiFiConfiguration::ProcessConfigFlags(msg.appConCfgFlags);
            break;
          }
          case RobotInterface::EngineToRobot::Tag_appConCfgIPInfo:
          {
            WiFiConfiguration::ProcessConfigIPInfo(msg.appConCfgIPInfo);
            break;
          }
          case RobotInterface::EngineToRobot::Tag_appConGetRobotIP:
          {
            WiFiConfiguration::SendRobotIpInfo(msg.appConGetRobotIP.ifId);
            break;
          }
          case RobotInterface::EngineToRobot::Tag_wifiOff:
          {
            WiFiConfiguration::Off(msg.wifiOff.sleep);
            break;
          }
          case RobotInterface::EngineToRobot::Tag_bodyStorageContents:
          {
            CrashReporter::AcceptBodyStorage(msg.bodyStorageContents);
            break;
          }
          case RobotInterface::EngineToRobot::Tag_requestCrashReports:
          {
            CrashReporter::TriggerLogSend(msg.requestCrashReports.index);
            break;
          }
          case RobotInterface::EngineToRobot::Tag_shutdownRobot:
          {
            RobotInterface::EngineToRobot msgToRTIP;
            msg.tag = RobotInterface::EngineToRobot::Tag_setBodyRadioMode;
            msg.setBodyRadioMode.radioMode = BODY_IDLE_OPERATING_MODE;
            RTIP::SendMessage(msg);
            break;
          }
          default:
          {
            AnkiWarn( 137, "WiFi.Messages", 259, "Received message not expected here tag=%02x", 1, msg.tag);
          }
        }
      }
      
      void ProcessMessage(u8* buffer, u16 bufferSize)
      {
        AnkiConditionalWarnAndReturn(buffer[0] <= RobotInterface::TO_WIFI_END, 364, "WiFi.Messages.ProcessMessage.AddressedToEngine", 394, "ToRobot message %x[%d] like like it has tag for engine (> 0x%x)", 3, buffer[0], bufferSize, (int)RobotInterface::TO_WIFI_END);
        AnkiConditionalWarnAndReturn(bufferSize <= RobotInterface::EngineToRobot::MAX_SIZE, 365, "WiFi.Messages.ProcessMessage.TooBig", 256, "Received message too big! %02x[%d] > %d", 3, buffer[0], bufferSize, RobotInterface::EngineToRobot::MAX_SIZE);
        if (buffer[0] < RobotInterface::TO_WIFI_START) // Message for someone further down than us
        {
          RTIP::SendMessage(buffer, bufferSize);
        }
        else if (RobotInterface::ANIM_BUFFER_START <= buffer[0] && buffer[0] <= RobotInterface::ANIM_BUFFER_END)
        {
          if(AnimationController::BufferKeyFrame(buffer, bufferSize) != RESULT_OK)
          {
            AnkiWarn( 137, "WiFi.Messages", 258, "Failed to buffer a keyframe! Clearing Animation buffer!\n", 0);
            AnimationController::Clear();
          }
        }
        else
        {
          RobotInterface::EngineToRobot msg;
          memcpy(msg.GetBuffer(), buffer, bufferSize);
          ProcessMessage(msg);
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
