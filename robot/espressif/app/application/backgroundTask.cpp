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
}
#include "face.h"

#define backgroundTaskQueueLen 2 ///< Maximum number of task 0 subtasks which can be in the queue
os_event_t backgroundTaskQueue[backgroundTaskQueueLen]; ///< Memory for the task 0 queue

#define EXPECTED_BT_INTERVAL_US 5000
#define BT_MAX_RUN_TIME_US      2000

namespace Anki {
namespace Cozmo {
namespace BackgroundTask {

void displayFaceCredentials(void)
{
  static bool wasConnected = true;
  if (clientConnected() && !wasConnected)
  {
    Face::FaceUnPrintf();
    wasConnected = true;
  }
  else if (!clientConnected() && wasConnected)
  {
    struct softap_config cfg;
    if (wifi_softap_get_config(&cfg))
    {
      Face::FacePrintf("%s\r\n%s\r\nChannel: %d\r\n", cfg.ssid, cfg.password, cfg.channel);
      wasConnected = false;
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
      displayFaceCredentials();
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
  else if (system_os_post(backgroundTask_PRIO, 0, 0) == false)
  {
    os_printf("\tCouldn't post background task initalization\r\n");
    return -1;
  }
  else
  {
    return 0;
  }
}
