/** Impelementation for task 1 module.
 * @author Daniel Casner <daniel@anki.com>
 */

#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "task1.h"

#define task1QueueLen 16 ///< Maximum number of task 1 subtasks which can be in the queue
os_event_t task1Queue[task1QueueLen]; ///< Memory for the task 1 queue

/** The OS task which dispatches subtasks.
*/
LOCAL void ICACHE_FLASH_ATTR task1Task(os_event_t *event)
{
  const foregroundTask subTask = (foregroundTask)(event->sig);
  if (subTask == NULL)
  {
    os_printf("ERROR: task1 sub task is NULL!\r\n");
  }
  else
  {
    const bool repost = subTask(event->par);
    if (repost)
    {
      system_os_post(TASK1_PRIO, (uint32)subTask, event->par);
    }
  }
}

int8_t ICACHE_FLASH_ATTR task1Init(void)
{
  os_printf("task1 init\r\n");
  if (system_os_task(task1Task, TASK1_PRIO, task1Queue, task1QueueLen) == false)
  {
    os_printf("\tCouldn't register OS task\r\n");
    return -1;
  }
  else
  {
    return 0;
  }
}

bool inline task1Post(foregroundTask task, uint32_t param)
{
  return system_os_post(TASK1_PRIO, (uint32)task, param);
}
