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
}
#include "anki/cozmo/robot/esp.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "anki/cozmo/robot/version.h"
#include "anki/cozmo/robot/logging.h"
#include "face.h"
#include "upgradeController.h"
#include "animationController.h"
#include "nvStorage.h"

#define backgroundTaskQueueLen 2 ///< Maximum number of task 0 subtasks which can be in the queue
os_event_t backgroundTaskQueue[backgroundTaskQueueLen]; ///< Memory for the task 0 queue

#define EXPECTED_BT_INTERVAL_US 5000
#define BT_MAX_RUN_TIME_US      2000

namespace Anki {
namespace Cozmo {
namespace BackgroundTask {

void CheckForUpgrades(void)
{
  const uint32 bodyCode  = i2spiGetBodyBootloaderCode();
  const uint16 bodyState = bodyCode & 0xffff;
  const uint16 bodyCount = bodyCode >> 16;
  if (((bodyState == STATE_IDLE) || (bodyState == STATE_NACK)) && (bodyCount > 10 && bodyCount < 100))
  {
    UpgradeController::StartBodyUpgrade();
  }
  else if (i2spiGetRtipBootloaderState() == STATE_IDLE)
  {
    UpgradeController::StartRTIPUpgrade();
  }
}

void WiFiFace(void)
{
  static const char wifiFaceFormat[] ICACHE_RODATA_ATTR STORE_ATTR = "%sSSID: %s\nPSK:  %s\nChan: %d  Stas: %d\nVer:  %x\nBy %s\nOn %s\n";
  const uint32 wifiFaceFmtSz = ((sizeof(wifiFaceFormat)+3)/4)*4;
  if (!clientConnected())
  {
    struct softap_config ap_config;
    if (wifi_softap_get_config(&ap_config) == false)
    {
      os_printf("WiFiFace couldn't read back config\r\n");
    }
    {
      char fmtBuf[wifiFaceFmtSz];
      char scrollLines[11];
      unsigned int i;
      memcpy(fmtBuf, wifiFaceFormat, wifiFaceFmtSz);
      for (i=0; i<((system_get_time()/2000000) % 10); i++) scrollLines[i] = '\n';
      scrollLines[i] = 0;
      Face::FacePrintf(fmtBuf, scrollLines,
                       ap_config.ssid, ap_config.password, ap_config.channel, wifi_softap_get_station_num(),
                       COZMO_VERSION_COMMIT, DAS_USER, BUILD_DATE);
    }
  }
}

void BootloaderDebugFace(void)
{
  Face::FacePrintf("RTIP: %04x\nBody: %08x", i2spiGetRtipBootloaderState(), i2spiGetBodyBootloaderCode());
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
    os_printf("Background task interval too long: %dus!\r\n", btInterval);
  }
  
  switch (event->sig)
  {
    case 0:
    {
      clientUpdate();
      break;
    }
    case 1:
    {
      CheckForUpgrades();
      break;
    }
    case 2:
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
      break;
    }
    case 3:
    {
      WiFiFace();
      //BootloaderDebugFace();
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
    os_printf("Background task run time too long: %dus!\r\n", btRunTime);
  }
  lastBTT = btStart;
  // Always repost so we'll execute again.
  system_os_post(backgroundTask_PRIO, event->sig + 1, event->par);
}


bool readCameraCalAndSend(uint32_t tag)
{
  NVStorage::NVStorageBlob entry;
  entry.tag = tag;
  const NVStorage::NVResult result = NVStorage::Read(entry);
  AnkiConditionalWarnAndReturnValue(result == NVStorage::NV_OKAY, false, 48, "ReadAndSendCameraCal", 272, "Failed to read camera calibration: %d", 1, result);
  const RobotInterface::CameraCalibration* const calib = (RobotInterface::CameraCalibration*)entry.blob;
  RobotInterface::SendMessage(*calib);
  return false;
}

} // BackgroundTask
} // Cozmo
} // Anki


extern "C" int8_t backgroundTaskInit(void)
{
  os_printf("backgroundTask init\r\n");
  if (system_os_task(Anki::Cozmo::BackgroundTask::Exec, backgroundTask_PRIO, backgroundTaskQueue, backgroundTaskQueueLen) == false)
  {
    os_printf("\tCouldn't register background OS task\r\n");
    return -1;
  }
  else if (Anki::Cozmo::AnimationController::Init() != Anki::RESULT_OK)
  {
    os_printf("\tCouldn't initalize animation controller\r\n");
    return -2;
  }
  else if (system_os_post(backgroundTask_PRIO, 0, 0) == false)
  {
    os_printf("\tCouldn't post background task initalization\r\n");
    return -3;
  }
  else if (Anki::Cozmo::Face::Init() != Anki::RESULT_OK)
  {
    os_printf("\tCouldn't initalize face controller\r\n");
    return -4;
  }
  else
  {
    return 0;
  }
}

extern "C" void backgroundTaskOnConnect(void)
{
  uint8_t msg[2] = {0xFC, true}; // FC is the tag for a radio connection state message to the robot
  i2spiQueueMessage(msg, 2);
  Anki::Cozmo::Face::FaceUnPrintf();
  Anki::Cozmo::AnimationController::ClearNumBytesPlayed();
  Anki::Cozmo::AnimationController::ClearNumAudioFramesPlayed();
  foregroundTaskPost(Anki::Cozmo::BackgroundTask::readCameraCalAndSend, Anki::Cozmo::NVStorage::NVEntry_CameraCalibration);
}

extern "C" void backgroundTaskOnDisconnect(void)
{
  uint8_t msg[2] = {0xFC, false}; // FC is the tag for a radio connection state message to the robot
  i2spiQueueMessage(msg, 2);
}
