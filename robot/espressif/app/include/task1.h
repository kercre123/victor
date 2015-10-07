/** @ file Header file for task1 (second priority / forground) task
 * @author Daniel Casner <daniel@anki.com>
 * The Espressif OS only suports 3 tasks 0, 1, 2. We have reserved 1 and 2 for specific time critical functions.
 */
#ifndef __task1_h
#define __task1_h

#define TASK1_PRIO USER_TASK_PRIO_1

/** Initalize the task 1 structures.
 * Must be called before any other functions in this module can be used.
 * @return 0 on success or non-zero on an error
 */
int8_t task1Init(void);

/** Prototype for task 1 sub tasks
 * param will be passed to the subTask when it is eventually called.
 * If true is returned, the task will be automatically reposted. If false it will not.
 */
typedef bool (*foregroundTask)(uint32_t param);

/** Post a task 1 subtask to the queue.
 * The task 1 queue is a FIFO with no prioritization, tasks will be executed in the order they are received when there
 * is no other code which needs to execute. They will not be pre-empted once started except by interrupts so they must
 * return quickly (optionally re-posting themselves) lest other tasks not be serviced or the watch-dog bites.
 * @param task a Pointer to the function to queue
 * @param param An argument to be passed along to the function when called.
 */
bool task1Post(foregroundTask task, uint32_t param);

#endif
