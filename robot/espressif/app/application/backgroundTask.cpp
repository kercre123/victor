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
#include "rboot.h"
}
#include "rtip.h"
#include "anki/cozmo/robot/esp.h"
#include "clad/robotInterface/messageToActiveObject.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot_hash.h"
#include "clad/robotInterface/messageRobotToEngine_hash.h"
#include "anki/cozmo/robot/logging.h"
#include "face.h"
#include "upgradeController.h"
#include "animationController.h"
#include "nvStorage.h"

#define backgroundTaskQueueLen 2 ///< Maximum number of task 0 subtasks which can be in the queue
os_event_t backgroundTaskQueue[backgroundTaskQueueLen]; ///< Memory for the task 0 queue

#define EXPECTED_BT_INTERVAL_US 5000
#define BT_MAX_RUN_TIME_US      2000

extern const unsigned int COZMO_VERSION_COMMIT;
extern const char* DAS_USER;
extern const char* BUILD_DATE;

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
  static const char wifiFaceFormat[] ICACHE_RODATA_ATTR STORE_ATTR = "SSID: %s\nPSK:  %s\nChan: %d  Stas: %d\nWiFi-V: %x\nWiFi-D: %s\nRTIP-V: %x\nRTIP-D: %s\n          %c";
  static const u32 wifiFaceSpinner[] ICACHE_RODATA_ATTR STORE_ATTR = {'|', '/', '-', '\\', '|', '/', '-', '\\'};
  const uint32 wifiFaceFmtSz = ((sizeof(wifiFaceFormat)+3)/4)*4;
  if (!clientConnected())
  {
    if (!crashHandlerHasReport())
    {
      struct softap_config ap_config;
      if (wifi_softap_get_config(&ap_config) == false)
      {
        os_printf("WiFiFace couldn't read back config\r\n");
      }
      char fmtBuf[wifiFaceFmtSz];
      memcpy(fmtBuf, wifiFaceFormat, wifiFaceFmtSz);
      Face::FaceInvertPrintf((system_get_time() >> 24) & 0x1); // Invert the face every few seconds
      Face::FacePrintf(fmtBuf,
                       ap_config.ssid, ap_config.password, ap_config.channel, wifi_softap_get_station_num(),
                       COZMO_VERSION_COMMIT, BUILD_DATE + 5,
                       RTIP::Version, RTIP::VersionDescription, wifiFaceSpinner[system_get_time() >> 16 & 0x7]);
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
    AnkiWarn( 51, "BackgroundTask.IntervalTooLong", 295, "Background task interval too long: %dus!", 1, btInterval);
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
    AnkiWarn( 52, "BackgroundTask.RunTimeTooLong", 296, "Background task run time too long: %dus!", 1, btRunTime);
  }
  lastBTT = btStart;
  // Always repost so we'll execute again.
  system_os_post(backgroundTask_PRIO, event->sig + 1, event->par);
}

bool readPairedObjectsAndSend(uint32_t tag)
{
  NVStorage::NVStorageBlob entry;
  entry.tag = tag;
  const NVStorage::NVResult result = NVStorage::Read(entry);
  AnkiConditionalWarnAndReturnValue(result == NVStorage::NV_OKAY, false, 48, "ReadAndSendPairedObjects", 272, "Failed to paired objects: %d", 1, result);
  CubeSlots* slots;
  // XXX TODO Remove this hack once the old robots are gone
  if (entry.blob_length == CubeSlots::MAX_SIZE) // New style CubeSlots message
  {
    slots = (CubeSlots*)entry.blob;
  }
  else // Old style slots message
  {
    AnkiWarn( 48, "ReadAndSendPairedObjects", 397, "Old style NV data found, please update", 0)
    slots = (CubeSlots*)(entry.blob + 4); // Skip past padding and length field
  }
  RobotInterface::EngineToRobot m;
  m.tag = RobotInterface::EngineToRobot::Tag_assignCubeSlots;
  memcpy(&m.assignCubeSlots, slots->GetBuffer(), slots->Size());
  
  RTIP::SendMessage(m);
  return false;
}

bool readCameraCalAndSend(uint32_t tag)
{
  NVStorage::NVStorageBlob entry;
  entry.tag = tag;
  const NVStorage::NVResult result = NVStorage::Read(entry);
  AnkiConditionalWarnAndReturnValue(result == NVStorage::NV_OKAY, false, 96, "ReadAndSendCameraCal", 350, "Failed to read camera calibration: %d", 1, result);
  const RobotInterface::CameraCalibration* const calib = (RobotInterface::CameraCalibration*)entry.blob;
  RobotInterface::SendMessage(*calib);
  return false;
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
  os_printf("backgroundTask init\r\n");
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
  else
  {
    return 0;
  }
}

extern "C" bool i2spiSynchronizedCallback(uint32 param)
{
  if (Anki::Cozmo::UpgradeController::CheckForAndDoStaged()) return false;
  foregroundTaskPost(Anki::Cozmo::BackgroundTask::readPairedObjectsAndSend, Anki::Cozmo::NVStorage::NVEntry_PairedObjects);
  return false;
}

extern "C" void backgroundTaskOnConnect(void)
{
  const uint32_t* const serialNumber = (const uint32_t* const)(FLASH_MEMORY_MAP + FACTORY_SECTOR*SECTOR_SIZE);
  
  if (crashHandlerHasReport()) foregroundTaskPost(Anki::Cozmo::BackgroundTask::readAndSendCrashReport, 0);
  else Anki::Cozmo::Face::FaceUnPrintf();
  
  // Tell RTIP radio is connected
  {
    Anki::Cozmo::RobotInterface::EngineToRobot rtipMsg;
    rtipMsg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_radioConnected;
    rtipMsg.radioConnected.wifiConnected = true;
    Anki::Cozmo::RTIP::SendMessage(rtipMsg);
  }
    
  // Send our version information to the engine
  {
    Anki::Cozmo::RobotInterface::FWVersionInfo vi;
    vi.wifiVersion = COZMO_VERSION_COMMIT;
    vi.rtipVersion = Anki::Cozmo::RTIP::Version;
    vi.bodyVersion = 0; // Don't have access to this yet
    os_memcpy(vi.toRobotCLADHash,  messageEngineToRobotHash, sizeof(vi.toRobotCLADHash));
    os_memcpy(vi.toEngineCLADHash, messageRobotToEngineHash, sizeof(vi.toEngineCLADHash));
    Anki::Cozmo::RobotInterface::SendMessage(vi);
  }
  
  // Send our serial number to the engine
  {
    Anki::Cozmo::RobotInterface::RobotAvailable idMsg;
    idMsg.robotID = *serialNumber;
    Anki::Cozmo::RobotInterface::SendMessage(idMsg);
  }
    
  Anki::Cozmo::AnimationController::Clear();
  Anki::Cozmo::AnimationController::ClearNumBytesPlayed();
  Anki::Cozmo::AnimationController::ClearNumAudioFramesPlayed();

  foregroundTaskPost(Anki::Cozmo::BackgroundTask::readPairedObjectsAndSend, Anki::Cozmo::NVStorage::NVEntry_PairedObjects);
  foregroundTaskPost(Anki::Cozmo::BackgroundTask::readCameraCalAndSend, Anki::Cozmo::NVStorage::NVEntry_CameraCalibration);
}

extern "C" void backgroundTaskOnDisconnect(void)
{
  Anki::Cozmo::RobotInterface::EngineToRobot rtipMsg;
  
  rtipMsg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_radioConnected;
  rtipMsg.radioConnected.wifiConnected = false;
  Anki::Cozmo::RTIP::SendMessage(rtipMsg);
  
  rtipMsg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_assignCubeSlots;
  os_memset(&rtipMsg.assignCubeSlots, 0, sizeof(Anki::Cozmo::CubeSlots));
  Anki::Cozmo::RTIP::SendMessage(rtipMsg);
}
