/** Impelementation for Task 0 module.
 * @author Daniel Casner <daniel@anki.com>
 */

extern "C" {
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "task0.h"
}

#define task0QueueLen 2 ///< Maximum number of task 0 subtasks which can be in the queue
os_event_t task0Queue[task0QueueLen]; ///< Memory for the task 0 queue

/** The OS task which dispatches subtasks.
*/
LOCAL void ICACHE_FLASH_ATTR task0Task(os_event_t *event)
{
  
  // Always repost so we'll execute again.
  system_os_post(TASK0_PRIO, event->sig + 1, event->par);
}

extern "C" int8_t ICACHE_FLASH_ATTR task0Init(void)
{
  os_printf("Task0 init\r\n");
  if (system_os_task(task0Task, TASK0_PRIO, task0Queue, task0QueueLen) == false)
  {
    os_printf("\tCouldn't register background OS task\r\n");
    return -1;
  }
  else if (system_os_post(TASK0_PRIO, 0, 0) == false)
  {
    os_printf("\tCouldn't post background task initalization\r\n");
    return -1;
  }
  else
  {
    return 0;
  }
}
