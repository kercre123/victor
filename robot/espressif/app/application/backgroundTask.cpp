/** Impelementation for background task module.
 * @author Daniel Casner <daniel@anki.com>
 */

extern "C" {
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "foregroundTask.h"
#include "backgroundTask.h"
#include "client.h"
#include "driver/i2spi.h"
#include "driver/crash.h"
}
#include "rtip.h"
#include "face.h"
#include "dhTask.h"
#include "factoryTests.h"
#include "nvStorage.h"
#include "wifi_configuration.h"
#include "anki/cozmo/robot/esp.h"
#include "clad/robotInterface/messageToActiveObject.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"
#include "clad/types/imageTypes.h"
#include "anki/cozmo/robot/logging.h"
#include "upgradeController.h"
#include "animationController.h"
#include "crashReporter.h"

#define backgroundTaskQueueLen 2 ///< Maximum number of task 0 subtasks which can be in the queue
os_event_t backgroundTaskQueue[backgroundTaskQueueLen]; ///< Memory for the task 0 queue

#define EXPECTED_BT_INTERVAL_US 5000
#define BT_MAX_RUN_TIME_US      2000

namespace Anki {
namespace Cozmo {
namespace BackgroundTask {
  

void RadioConnectionStateMachineUpdate()
{
  static u8 lastStaCount = 0;
  static u8 lastConCount = 0;
  static s8 doRTConnectPhase = 0;
  static s8 doRTDisconnectPhase = 0;
  static bool sendRadioState = true; // Need to send once just to say enabled
  
  const u8 currentStaCount = wifi_softap_get_station_num();
  const u8 currentConCount = clientConnected();
    
  if ((lastStaCount == 0) && (currentStaCount  > 0)) // First station connected
  {
    sendRadioState = true;
  }
  
  if ((lastConCount == 0) && (currentConCount  > 0)) // First reliable transport connection
  {
    sendRadioState = true;
    doRTConnectPhase    = 1;
    doRTDisconnectPhase = 0;
  }
  else if ((lastConCount  > 0) && (currentConCount == 0)) // Last reliable transport disconnection
  {
    sendRadioState = true;
    doRTConnectPhase    = 0;
    doRTDisconnectPhase = 1;
  }
  
  if (sendRadioState)
  {
    WiFiState rws;
    rws.enabled = true;
    rws.staCount = currentStaCount;
    rws.rtCount  = currentConCount;
    if (RobotInterface::SendMessage(rws)) sendRadioState = false;
  }
  else if (doRTConnectPhase)
  {
    switch (doRTConnectPhase)
    {
      case 1:
      {
        Factory::SetMode(RobotInterface::FTM_None);
        doRTConnectPhase++;
        break;
      }
      case 2:
      {
        RobotInterface::RobotAvailable idMsg;
        idMsg.robotID = getSerialNumber();
        idMsg.modelID = getModelNumber();
        if (RobotInterface::SendMessage(idMsg)) doRTConnectPhase++;
        break;
      }
      case 3:
      {
        RobotInterface::FirmwareVersion versionMsg;
        STACK_LEFT(true);
        u32* versionInfoAsU32 = reinterpret_cast<u32*>(versionMsg.json);
        os_memcpy(versionInfoAsU32,
                  Anki::Cozmo::UpgradeController::GetVersionInfo(),
                  Anki::Cozmo::UpgradeController::VERSION_INFO_MAX_LENGTH);
        versionMsg.json_length = os_strlen((const char*)versionMsg.json);
        if (clientQueueAvailable() > ((int)versionMsg.Size() + LOW_PRIORITY_BUFFER_ROOM))
        {
          if (RobotInterface::SendMessage(versionMsg)) doRTConnectPhase++;
        }
        break;
      }
      case 4:
      {
        doRTConnectPhase = 0; // Done
        break;
      }
    }
  }
  else if (doRTDisconnectPhase)
  {
    switch (doRTDisconnectPhase)
    {
      case 1:
      {
        AnimationController::EngineDisconnect();
        Anki::Cozmo::UpgradeController::OnDisconnect();
        doRTDisconnectPhase++;
        break;
      }
      case 2:
      {
        if (Anki::Cozmo::Factory::GetMode() == Anki::Cozmo::RobotInterface::FTM_None)
        {
          Anki::Cozmo::Factory::SetMode(Anki::Cozmo::RobotInterface::FTM_entry);
          Anki::Cozmo::Face::Clear();
        }
        doRTDisconnectPhase++;
        break;
      }
      case 3:
      {
        sendRadioState = true;
        doRTDisconnectPhase = 0;
        break;
      }
    }
  }
  
  lastStaCount = currentStaCount;
  lastConCount = currentConCount;
}



/** The OS task which dispatches subtasks.
*/
void Exec(os_event_t *event)
{
  static u32 lastBTT = system_get_time();
  static u8 periodicPrint = 0;
  const u32 btStart  = system_get_time();
  const u32 btInterval = btStart - lastBTT;
  if ((btInterval > EXPECTED_BT_INTERVAL_US*2) && (periodicPrint++ == 0))
  {
    AnkiWarn( 51, "BackgroundTask.IntervalTooLong", 295, "Background task interval too long: %dus!", 1, btInterval);
  }

  RTIP::Update();

  switch (event->sig)
  {
    case 0:
    {
      RadioConnectionStateMachineUpdate();
      break;
    }
    case 1:
    {
      Factory::Update();
      break;
    }
    case 2:
    {
      NVStorage::Update();
      break;
    }
    case 3:
    {
      UpgradeController::Update();
      break;
    }
    case 4:
    {
      {
        static uint32_t lastPEC = 0;
        const uint32_t pec = i2spiGetPhaseErrorCount();
        if (lastPEC == 0xFFFFffff) system_deep_sleep(0); // Reported fatal error, now shutdown
        else
        {
          if (pec > lastPEC) 
          {
            AnkiWarn( 185, "I2SPI.TooMuchDrift", 486, "TMD=%d\tintegral=%d", 2, pec, i2spiGetIntegralDrift());
            if (pec > 2)
            {
              RobotInterface::RobotErrorReport rer;
              rer.error = RobotInterface::REC_I2SPI_TMD;
              rer.fatal = true;
              if (RobotInterface::SendMessage(rer)) lastPEC = 0xFFFFffff; // Mark reported
            }
          }
          lastPEC = pec;
        }
      }
      {
        static uint32_t lastDropCount = 0;
        const uint32_t dc = clientDropCount();
        if ((dc > lastDropCount) && clientQueueAvailable() > LOW_PRIORITY_BUFFER_ROOM)
        {
          RobotInterface::RobotErrorReport rer;
          rer.error = RobotInterface::REC_ReliableTransport;
          rer.fatal = false;
          if (RobotInterface::SendMessage(rer)) lastDropCount = dc;
          AnkiWarn( 370, "client.reliable_message_dropped", 607, "count = %d", 1, dc);
        }
      }
      break;
    }
    case 5:
    {
      CrashReporter::Update();
      break;
    }
    case 6:
    {
      AnimationController::Update();
      break;
    }
    case 7:
    {
      Face::Update();
      break;
    }
    // Add new "long execution" tasks as switch cases here.
    default:
    {
      event->sig = -1; // So next call will be event 0
    }
  }

  clientUpdate(); // Do this last to send all the messages we just generated

  const u32 btRunTime = system_get_time() - btStart;
  if ((btRunTime > BT_MAX_RUN_TIME_US) && (periodicPrint++ == 0))
  {
    AnkiWarn( 52, "BackgroundTask.RunTimeTooLong", 296, "Background task run time too long: %dus!", 1, btRunTime);
  }
  lastBTT = btStart;
  // Always repost so we'll execute again.
  system_os_post(backgroundTask_PRIO, event->sig + 1, event->par);
}

} // BackgroundTask
} // Cozmo
} // Anki


extern "C" int8_t backgroundTaskInit(void)
{
  //os_printf("backgroundTask init\r\n");
  if (system_os_task(Anki::Cozmo::BackgroundTask::Exec, backgroundTask_PRIO, backgroundTaskQueue, backgroundTaskQueueLen) == false)
  {
    os_printf("\tCouldn't register background OS task\r\n");
    return -1;
  }
  else if (DiffieHellman::Init() != true)
  {
    os_printf("\tCouldn't initalize Diffie Hellman module\r\n");
    return -2;
  }
  else if (Anki::Cozmo::RTIP::Init() != true)
  {
    os_printf("\tCouldn't initalize RTIP interface module\r\n");
    return -2;
  }
  else if (Anki::Cozmo::AnimationController::Init() != Anki::RESULT_OK)
  {
    os_printf("\tCouldn't initalize animation controller\r\n");
    return -3;
  }
  else if (system_os_post(backgroundTask_PRIO, 0, 0) == false)
  {
    os_printf("\tCouldn't post background task initalization\r\n");
    return -4;
  }
  else if (Anki::Cozmo::Face::Init() != Anki::RESULT_OK)
  {
    os_printf("\tCouldn't initalize face controller\r\n");
    return -5;
  }
  else if (Anki::Cozmo::Factory::Init() == false)
  {
    os_printf("\tCouldn't initalize factory test framework\r\n");
    return -6;
  }
  else if (Anki::Cozmo::WiFiConfiguration::Init() != Anki::RESULT_OK)
  {
    os_printf("\tCouldn't initalize WiFiConfiguration module\r\n");
    return -7;
  }
  else if (Anki::Cozmo::CrashReporter::Init() == false)
  {
    os_printf("\tCouldn't initalize CrashReporter\r\n");
    return -8;
  }
  // Upgrade controller should be initalized last
  else if (Anki::Cozmo::UpgradeController::Init() == false)
  {
    os_printf("\tCouldn't initalize upgrade controller\r\n");
    return -128;
  }
  else
  {
    return 0;
  }
}

extern "C" bool i2spiSynchronizedCallback(uint32 param)
{
  os_printf("I2SPI Synchronized at offset %d\r\n", param);
  Anki::Cozmo::Factory::SetMode(Anki::Cozmo::RobotInterface::FTM_entry);
  Anki::Cozmo::CrashReporter::StartQuery();
  return false;
}

extern "C" void i2spiResyncCallback(void)
{
  using namespace Anki::Cozmo::RobotInterface;
  RobotErrorReport rer;
  rer.error = REC_I2SPI_TMD;
  rer.fatal = false;
  SendMessage(rer);
  Anki::Cozmo::Face::ResetScreen();
  AnkiWarn( 371, "I2SPI.Resync", 608, "I2SPI resynchronizing", 0);
}
