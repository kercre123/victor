/** @file Header file for task2, highest priority execution cooperative thread
 * @author Daniel Casner <daniel@anki.com>
 */

#ifndef __task2_h
#define __task2_h

#define TASK2_PRIO USER_TASK_PRIO_2

/** Initalize task2 related structures
 * Must be called before the task is scheduled
 */
int8 task2Init(void);

/** Post task2 for execution
 * @param signal Signal to past to the scheduled execution
 * @param parameter Parameter to pass to the scheduled execution
 * @return true if the task was posted, false if there was an error.
 */
bool task2Post(uint32 signal, uint32 parameter);

#endif
