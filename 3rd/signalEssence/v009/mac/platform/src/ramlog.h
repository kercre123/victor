/**
 * A RAM based logger.  Very handy for tracking real time events.
 * currently uses linuxy services like semaphore, malloc, and timespec
 * for thread exclusion, memory allocation and timing.  But wouldn't
 * be too terrible to  make it work on other platforms.
 * 
 * Usage:
 * 
 * ramlog_t log; // global logger
 * main {
 *      ramlog_error_t err;
 *      int logsize = 1000;
 *      err = ramlog_init(&log, logsize);
 *      if (err != RAMLOG_OK) {
 *         // couldn't allocate memory...
 *      }    
 * }
 * // In any thread/process/interrupt handler:
 * ramlog_log(&log, <some ramlog_data_t item, whatever you want>);
 * // currently ramlog_data_t is just a 64-bit int, but could be
 * // a struct.
 * 
 * // You get to define a callback function that's called when you want
 * // to dump the log.
 * void logdump(long long unsigned int time_ns, ramlog_data_t *data) {
 *     // this  could be named anything, and the data has any format
 *     // you defined.   I do it thusly:
 *     char c = *data & 0xFF; // grab just the LSB, use it as a key:
 *     uint_8 b1 = (*data >> 8) & 0xFF;
 *     uint_8 b2 = (*data >> 16) & 0xFF;
 *     uint_8 b3 = (*data >> 24) & 0xFF;
 *     uint_16 w1 = (*data >> 32) & 0xFFFF;
 *     uint_16 w2 = (*data >> 48) & 0xFFFF; // 1 64-bit word is thusly
 *                                          // sliced up into different
 *                                          // args. 
 *     switch (c) {
 *        case 'a':  
 *               // do something with 'a' type data.
 *            fprintf("stuff: %c %d %d", c, b1, b2);
 *        break;
 *      }
 * }
 *
 * ramlog_dump(&log, logdump); // dump the data.
 *   // I call ramlog_dump from a wrapper function
 * dumper() {
 *    ramlog_dump(&log, logdump);
 * }
 * 
 * // This is so that its very easy to call 'dumper()' from gdb:
 * //   (gdb) call dumper()
 * //  will print the data out to stdout.
 * 
 * ramlog_deinit(&log);
 */
#ifndef _RAMLOG_H_
#define _RAMLOG_H_
#include <time.h>
#include <semaphore.h>
   
/**  ramlog_error_t 
 *  The only error that can happen so far is that the malloc doesn't
 *  succeed... 
 */
typedef enum {
    RAMLOG_OK = 0,
    RAMLOG_NOMEM = 1,
} ramlog_error_t;

/** 
 *  Define this to your liking. This is the basic data type that is
 *  perhaps a struct would be nicer.
*/
typedef long long int ramlog_data_t;

/** 
 * Internal data type.  Just ignore the contents unless you're
 * modifying ramlog itself.
 */
typedef struct {
    int              n_elements; // count of elements in data and times fields.
    struct timespec  time_start; // current time ptr
    struct timespec *times;
    ramlog_data_t   *data;
    struct timespec *ptimes; // current time ptr
    ramlog_data_t   *pdata;  // current data ptr
    int              current;// current element.  &times[current] == ptimes, etc.
    int              has_looped; // if has_looped, then all items are valid.  else, only elements up to current are valie.
    sem_t            sem;
} ramlog_t;

/** This is the type of a callback for the ramlog_dump function
 *  it's up to you to define a callback.
 *  @param time_ns is the time the log was called, in ns.
 *  @param data    is the data element.  
 *
 */
typedef void(*ramlog_callback)(long long unsigned int time_ns, ramlog_data_t *data);

/** Initialize the ramlog.
 * @param log   uninitialized pointer to a structure.
 * @param count the number of items to allocate.
 * @returns RAMLOG_OK on success, RAMLOG_NOMEM on alloc failure.
 */
ramlog_error_t ramlog_init(ramlog_t *log, int count);
/** deinitialize the ram log.
 *  just frees any memory allocated.
 * @param log pointer to the log
 * @returns RAMLOG_OK
 */
ramlog_error_t ramlog_deinit(ramlog_t *log);

/** 
 * logs the data, along with the time.
 * @param log the log
 * @param data the data
 */
ramlog_error_t ramlog_log(ramlog_t *log, const ramlog_data_t data);

/** calls 'callback' for each logged item. 
 *  @param log the log
 *  @param callback callback function that's called for each logged
 *  item.  The callback should be of the type:
 *     void cb_name(long long int time, ramlog_data_t *data);
 */
ramlog_error_t ramlog_dump(ramlog_t *log, ramlog_callback callback);
#endif
