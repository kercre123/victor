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
#include "driver/factoryData.h"  
}
#include "rtip.h"
#include "face.h"
#include "bluetoothTask.h"
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
  static u8 lastConCount = 0;
  static s8 doRTConnectPhase = 0;
  static s8 doRTDisconnectPhase = 0;

  const u8 currentConCount = clientConnected();

  if ((lastConCount == 0) && (currentConCount  > 0))  // First reliable transport connection
  {
    doRTConnectPhase    = 1;
    doRTDisconnectPhase = 0;
  }
  else if ((lastConCount  > 0) && (currentConCount == 0))  // Last reliable transport disconnection
  {
    doRTConnectPhase    = 0;
    doRTDisconnectPhase = 1;
  }
  
  i2spiSetAppConnected(currentConCount > 0);

  if (doRTConnectPhase)
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
        idMsg.hwRevision = getHardwareRevision();
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
        uint8 macaddr[6];
        if (wifi_get_macaddr(SOFTAP_IF, macaddr))
        {
          AnkiEvent( 409, "macaddr.soft_ap", 624, "%02x:%02x:%02x:%02x:%02x:%02x", 6, macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
        }
        else
        {
          AnkiWarn( 410, "macaddr.soft_ap.error", 625, "Unable to retrieve softap MAC address", 0);
        }
        doRTConnectPhase++;
        break;
      }
      case 5:
      {
        struct dhcps_lease dhcpInfo;
        if (wifi_softap_get_dhcps_lease(&dhcpInfo))
        {
          AnkiEvent( 411, "boot_count.phase", 626, "%x", 1, (dhcpInfo.start_ip.addr >> 24));
        }
        else
        {
          AnkiWarn( 412, "boot_count.error", 627, "Unable to retrieve boot phase indicator", 0);
        }
        doRTConnectPhase++;
        break;
      }
      case 6:
      {
        AnkiEvent( 413, "client.connections_since_boot", 347, "%d", 1, clientConnectCount());
        doRTConnectPhase++;
        break;
      }
      case 7:
      {
        AnkiWarn( 1203, "hardware.revision", 632, "Hardware 1.%d", 1, getHardwareRevision()&0xFF);
        doRTConnectPhase++;
        break;
      }
      case 8:
      {
        RobotInterface::WiFiFlashID fidMsg;
        fidMsg.chip_id = spi_flash_get_id();
        RobotInterface::SendMessage(fidMsg);
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
        Messages::ResetMissedLogCount();
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
        doRTDisconnectPhase = 0;
        break;
      }
    }
  }

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
      if (true)
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
      if (true)
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
      if (true)
      {
        int32_t data;
        const I2SPIError i2spiErr = i2spiGetErrorCode(&data);
        AnkiConditionalWarn(i2spiErr == I2SPIE_None, 403, "I2SPI.Error", 622, "code=%x, data=%x", 2, i2spiErr, data);
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
    AnkiEvent( 52, "BackgroundTask.RunTimeTooLong", 296, "Background task %d run time too long: %dus!", 2, event->sig, btRunTime);
  }
  lastBTT = btStart;
  // Always repost so we'll execute again.
  system_os_post(backgroundTask_PRIO, event->sig + 1, event->par);
}

} // BackgroundTask
} // Cozmo
} // Anki

enum BackgroundTaskError
{
  BTE_ok = 0,
  BTE_exec_task = -1,
  BTE_ble_init = -2,
  BTE_rtip_init = -3,
  BTE_anim_init = -4,
  BTE_bg_post = -5,
  BTE_face_init = -6,
  BTE_factory_init = -7,
  BTE_wifi_init = -8,
  BTE_crash_init = -9,
  BTE_storage_init = -10,
  BTE_upgrade_init = -127
};

extern "C" int8_t backgroundTaskInit(void)
{
  //os_printf("backgroundTask init\r\n");
  BackgroundTaskError result = BTE_ok;
  if (system_os_task(Anki::Cozmo::BackgroundTask::Exec, backgroundTask_PRIO, backgroundTaskQueue, backgroundTaskQueueLen) == false)
  {
    os_printf("\tCouldn't register background OS task\r\n");
    result =  BTE_exec_task;
  }
  else if (Bluetooth::Init() != true)
  {
    os_printf("\tCouldn't initalize BLE module\r\n");
    result =  BTE_ble_init;
  }
  else if (Anki::Cozmo::RTIP::Init() != true)
  {
    os_printf("\tCouldn't initalize RTIP interface module\r\n");
    result =  BTE_rtip_init;
  }
  else if (Anki::Cozmo::AnimationController::Init() != Anki::RESULT_OK)
  {
    os_printf("\tCouldn't initalize animation controller\r\n");
    result =  BTE_anim_init;
  }
  else if (system_os_post(backgroundTask_PRIO, 0, 0) == false)
  {
    os_printf("\tCouldn't post background task initalization\r\n");
    result =  BTE_bg_post;
  }
  else if (Anki::Cozmo::Face::Init() != Anki::RESULT_OK)
  {
    os_printf("\tCouldn't initalize face controller\r\n");
    result =  BTE_face_init;
  }
  else if (Anki::Cozmo::Factory::Init() == false)
  {
    os_printf("\tCouldn't initalize factory test framework\r\n");
    result =  BTE_factory_init;
  }
  else if (Anki::Cozmo::WiFiConfiguration::Init() != Anki::RESULT_OK)
  {
    os_printf("\tCouldn't initalize WiFiConfiguration module\r\n");
    result =  BTE_wifi_init;
  }
  else if (Anki::Cozmo::CrashReporter::Init() == false)
  {
    os_printf("\tCouldn't initalize CrashReporter\r\n");
    result =  BTE_crash_init;
  }
  else if (Anki::Cozmo::NVStorage::Init() == false)
  {
    os_printf("\tCouldn't initalize NVStorage\r\n");
    result =  BTE_storage_init;
  }
  // Upgrade controller should be initalized last
  else if (Anki::Cozmo::UpgradeController::Init() == false)
  {
    os_printf("\tCouldn't initalize upgrade controller\r\n");
    result =  BTE_upgrade_init;
  }
  if (result)
  {
    recordBootError((void*)backgroundTaskInit, result);
  }
  return (int8_t)result;
}

extern "C" bool i2spiSynchronizedCallback(uint32 param)
{
  os_printf("I2SPI Synchronized at offset %d\r\n", param);
  os_delay_us(40000); // Need to wait some time for i2spi to be ready for CLAD messages
  clientAccept(true);
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
