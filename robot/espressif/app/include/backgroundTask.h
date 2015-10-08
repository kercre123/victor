/** @ file Header file for backgroundTask (lowest priority / background / whatever) task
 * @author Daniel Casner <daniel@anki.com>
 * The Espressif OS only suports 3 tasks 0, 1, 2. We have reserved 1 and 2 for specific time critical functions.
 * Task 0 is used for background structures.
 */
#ifndef __backgroundTask_h
#define __backgroundTask_h

#define backgroundTask_PRIO USER_TASK_PRIO_0

/** Initalize the backgroundTask structures.
 * Must be called before any other functions in this module can be used.
 * @return 0 on success or non-zero on an error
 */
int8_t backgroundTaskInit(void);

#endif
