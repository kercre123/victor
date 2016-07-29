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
#include "activeObjectManager.h"
#include "face.h"
#include "factoryTests.h"
#include "nvStorage.h"
#include "wifi_configuration.h"
#include "anki/cozmo/robot/esp.h"
#include "clad/robotInterface/messageToActiveObject.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot_hash.h"
#include "clad/robotInterface/messageRobotToEngine_hash.h"
#include "clad/types/imageTypes.h"
#include "anki/cozmo/robot/logging.h"
#include "upgradeController.h"
#include "animationController.h"
#include "../factoryversion.h"

#define backgroundTaskQueueLen 2 ///< Maximum number of task 0 subtasks which can be in the queue
os_event_t backgroundTaskQueue[backgroundTaskQueueLen]; ///< Memory for the task 0 queue

#define EXPECTED_BT_INTERVAL_US 5000
#define BT_MAX_RUN_TIME_US      2000

namespace Anki {
namespace Cozmo {
namespace BackgroundTask {

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
  
  clientUpdate();
  
  switch (event->sig)
  {
    case 0:
    {
      if (clientConnected())
      {
        static u32 lastAnimStateTime = 0;
        const u32 now = system_get_time();
        if ((now - lastAnimStateTime) > ANIM_STATE_INTERVAL)
        {
          if (AnimationController::SendAnimStateMessage() == RESULT_OK)
          {
            lastAnimStateTime = now;
          }
        }
      }
      break;
    }
    case 1:
    {
      Factory::Update();
      break;
    }
    case 2:
    {
      if (FACTORY_FIRMWARE == 0)
      {
        ActiveObjectManager::Update();
      }
      break;
    }
    case 3:
    {
      NVStorage::Update();
      break;
    }
    case 4:
    {
      UpgradeController::Update();
      break;
    }
    case 5:
    {
      static uint32_t lastErrorReport = 0;
      if (i2spiPhaseErrorCount - lastErrorReport > 2)
      {
        lastErrorReport = i2spiPhaseErrorCount;
        AnkiWarn( 185, "I2SPI.TooMuchDrift", 486, "TMD=%d\tintegral=%d", 2, i2spiPhaseErrorCount, i2spiIntegralDrift);
      }
      break;
    }
    // Add new "long execution" tasks as switch cases here.
    default:
    {
      event->sig = -1; // So next call will be event 0
    }
  }
  
  const u32 btRunTime = system_get_time() - btStart;
  if ((btRunTime > BT_MAX_RUN_TIME_US) && (periodicPrint++ == 0))
  {
    AnkiWarn( 52, "BackgroundTask.RunTimeTooLong", 296, "Background task run time too long: %dus!", 1, btRunTime);
  }
  lastBTT = btStart;
  // Always repost so we'll execute again.
  system_os_post(backgroundTask_PRIO, event->sig + 1, event->par);
}

bool readAndSendCrashReport(uint32_t param)
{
  RobotInterface::CrashReport crMsg;
  crMsg.which = RobotInterface::WiFiCrash;
  if (crashHandlerGetReport(crMsg.dump, crMsg.MAX_SIZE) > 0)
  {
    if (RobotInterface::SendMessage(crMsg))
    {
      crashHandlerClearReport();
      return false;
    }
  }
  return true;
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
  else if (Anki::Cozmo::ActiveObjectManager::Init() == false)
  {
    os_printf("\tCouldn't initalize prop manager\r\n");
    return -6;
  }
  else if (Anki::Cozmo::Factory::Init() == false)
  {
    os_printf("\tCouldn't initalize factory test framework\r\n");
    return -7;
  }
  else if (Anki::Cozmo::WiFiConfiguration::Init() != Anki::RESULT_OK)
  {
    os_printf("\tCouldn't initalize WiFiConfiguration module\r\n");
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
  // Tell body / k02 to go into gameplay power mode
  #if !FACTORY_FIRMWARE
  {
    Anki::Cozmo::RobotInterface::SetBodyRadioMode bMsg;
    bMsg.radioMode = Anki::Cozmo::RobotInterface::BODY_ACCESSORY_OPERATING_MODE;
    Anki::Cozmo::RobotInterface::SendMessage(bMsg);

    Anki::Cozmo::RobotInterface::SetHeadDeviceLock hMsg;
    hMsg.enable = true;
    Anki::Cozmo::RobotInterface::SendMessage(hMsg);
  }
  #endif

  Anki::Cozmo::Factory::SetMode(Anki::Cozmo::RobotInterface::FTM_entry);
  return false;
}

static bool sendWifiConnectionState(const bool state)
{
  Anki::Cozmo::RobotInterface::RadioState rtipMsg;
  rtipMsg.wifiConnected = state;
  return Anki::Cozmo::RobotInterface::SendMessage(rtipMsg);
}

extern "C" void backgroundTaskOnConnect(void)
{
  using namespace Anki::Cozmo;
  if (crashHandlerHasReport()) foregroundTaskPost(BackgroundTask::readAndSendCrashReport, 0);
  else Anki::Cozmo::Face::FaceUnPrintf();

  if (FACTORY_FIRMWARE && (Anki::Cozmo::Factory::GetMode() == RobotInterface::FTM_PlayPenTest))
  {
    RobotInterface::FactoryTestParameter ftp;
    ftp.param = Anki::Cozmo::Factory::GetParam();
    Anki::Cozmo::RobotInterface::SendMessage(ftp);
  }
  else
  {
    Factory::SetMode(RobotInterface::FTM_None);
  }
  
  sendWifiConnectionState(true);
  
  if (FACTORY_FIRMWARE)
  {
    AnkiEvent( 186, "FactoryFirmware", 487, "Running factory firmware, EP3F", 0);
  }
  
  // Send our version information to the engine
  {
    RobotInterface::FWVersionInfo vi;
    vi.wifiVersion = FACTORY_VERSION; // COZMO_VERSION_COMMIT;
    vi.rtipVersion = FACTORY_VERSION; // RTIP::Version;
    vi.bodyVersion = 0; // Don't have access to this yet
    os_memcpy(vi.toRobotCLADHash,  messageEngineToRobotHash, sizeof(vi.toRobotCLADHash));
    os_memcpy(vi.toEngineCLADHash, messageRobotToEngineHash, sizeof(vi.toEngineCLADHash));
    RobotInterface::SendMessage(vi);
  }

  // Send our serial number to the engine
  {
    RobotInterface::RobotAvailable idMsg;
    idMsg.robotID = getSerialNumber();
    idMsg.modelID = getModelNumber();
    RobotInterface::SendMessage(idMsg);
  }

  AnimationController::Clear();
  AnimationController::ClearNumBytesPlayed();
  AnimationController::ClearNumAudioFramesPlayed();
}

extern "C" void backgroundTaskOnDisconnect(void)
{
  Anki::Cozmo::ActiveObjectManager::DisconnectAll();
  Anki::Cozmo::UpgradeController::OnDisconnect();
  sendWifiConnectionState(false);
  if (FACTORY_FIRMWARE && (Anki::Cozmo::Factory::GetMode() == Anki::Cozmo::RobotInterface::FTM_PlayPenTest))
  {
    i2spiSwitchMode(I2SPI_SHUTDOWN);
  }
  else if (Anki::Cozmo::Factory::GetMode() == Anki::Cozmo::RobotInterface::FTM_None)
  {
    Anki::Cozmo::Factory::SetMode(Anki::Cozmo::RobotInterface::FTM_entry);
    Anki::Cozmo::Face::Clear();
  }
}
