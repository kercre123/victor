/** Impelementation for task 1 module.
 * @author Daniel Casner <daniel@anki.com>
 */

#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "foregroundTask.h"

#define foregroundTaskQueueLen 16 ///< Maximum number of task 1 subtasks which can be in the queue
static os_event_t foregroundTaskQueue[foregroundTaskQueueLen]; ///< Memory for the task 1 queue

/** The OS task which dispatches subtasks.
*/
void foregroundTaskTask(os_event_t *event)
{
  const foregroundTask subTask = (foregroundTask)(event->sig);
  if (subTask == NULL)
  {
    os_printf("ERROR: foregroundTask sub task is NULL!\r\n");
  }
  else
  {
    const bool repost = subTask(event->par);
    if (repost)
    {
      if (system_os_post(foregroundTask_PRIO, event->sig, event->par) == false)
      {
        os_printf("Couldn't repost foregroundTask 0x%x\r\n", event->sig);
      }
    }
  }
}

int8_t ICACHE_FLASH_ATTR foregroundTaskInit(void)
{
  //os_printf("foregroundTask init\r\n");
  if (system_os_task(foregroundTaskTask, foregroundTask_PRIO, foregroundTaskQueue, foregroundTaskQueueLen) == false)
  {
    os_printf("\tCouldn't register OS task\r\n");
    return -1;
  }
  else
  {
    return 0;
  }
}
