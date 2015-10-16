/** Impelementation for background task module.
 * @author Daniel Casner <daniel@anki.com>
 */

extern "C" {
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "backgroundTask.h"
}

#define backgroundTaskQueueLen 2 ///< Maximum number of task 0 subtasks which can be in the queue
os_event_t backgroundTaskQueue[backgroundTaskQueueLen]; ///< Memory for the task 0 queue

#define EXPECTED_BT_INTERVAL_US 5000
#define BT_MAX_RUN_TIME_US      2000

/** The OS task which dispatches subtasks.
*/
LOCAL void ICACHE_FLASH_ATTR backgroundTaskExec(os_event_t *event)
{
  static u32 lastBTT = system_get_time();
  const u32 btStart  = system_get_time();
  const u32 btInterval = btStart - lastBTT;
  if (btInterval > EXPECTED_BT_INTERVAL_US*2) os_printf("Background task interval too long: %dus!\r\n", btInterval);
  
  
  
  const u32 btRunTime = system_get_time() - btStart;
  if (btRunTime > BT_MAX_RUN_TIME_US) os_printf("Background task run time too long: %dus!\r\n", btRunTime);
  // Always repost so we'll execute again.
  system_os_post(backgroundTask_PRIO, event->sig + 1, event->par);
}

extern "C" int8_t ICACHE_FLASH_ATTR backgroundTaskInit(void)
{
  os_printf("backgroundTask init\r\n");
  if (system_os_task(backgroundTaskExec, backgroundTask_PRIO, backgroundTaskQueue, backgroundTaskQueueLen) == false)
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
