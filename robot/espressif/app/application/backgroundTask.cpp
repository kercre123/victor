/** Impelementation for background task module.
 * @author Daniel Casner <daniel@anki.com>
 */

extern "C" {
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "backgroundTask.h"
#include "client.h"
#include "driver/i2spi.h"
}
#include "face.h"
#include "upgradeController.h"
#include "animationController.h"

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
  if (bodyState == STATE_IDLE && bodyCount > 10 && bodyCount < 100)
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
  static bool wasConnected = false;
  if (clientConnected() && !wasConnected)
  {
    Face::FaceUnPrintf();
    wasConnected = true;
  }
  else if (!clientConnected())
  {
    wasConnected = false;
    struct softap_config ap_config;
    if (wifi_softap_get_config(&ap_config) == false)
    {
      os_printf("WiFiFace couldn't read back config\r\n");
    }
    {
      Face::FacePrintf("SSID: %s\nPSK:  %s\nChan: %d\nStas: %d\n",
                       ap_config.ssid, ap_config.password, ap_config.channel, wifi_softap_get_station_num());
    }
  }
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

} // Background 
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
  else
  {
    return 0;
  }
}
