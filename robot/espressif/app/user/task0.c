/** Impelementation for Task 0 module.
 * @author Daniel Casner <daniel@anki.com>
 */

#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "task0.h"

#define task0QueueLen 16 ///< Maximum number of task 0 subtasks which can be in the queue
os_event_t task0Queue[task0QueueLen]; ///< Memory for the task 0 queue

/** The OS task which dispatches subtasks.
*/
LOCAL void ICACHE_FLASH_ATTR task0Task(os_event_t *event)
{
  const zeroSubTask subTask = (zeroSubTask)(event->sig);
  if (subTask == NULL)
  {
    os_printf("ERROR: task0 sub task is NULL!\r\n");
  }
  else
  {
    const bool repost = subTask(event->par);
    if (repost)
    {
      system_os_post(TASK0_PRIO, (uint32)subTask, event->par);
    }
  }
}

int8_t ICACHE_FLASH_ATTR task0Init(void)
{
  os_printf("Task0 init\r\n");
  if (system_os_task(task0Task, TASK0_PRIO, task0Queue, task0QueueLen) == false)
  {
    os_printf("\tCouldn't register OS task\r\n");
    return -1;
  }
  else
  {
    return 0;
  }
}

bool inline task0Post(zeroSubTask task, uint32_t param)
{
  return system_os_post(TASK0_PRIO, (uint32)task, param);
}
