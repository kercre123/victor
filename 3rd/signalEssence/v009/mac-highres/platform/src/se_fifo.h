/*
 * se_fifo.h
 *
 *
 * A simple, stupid, word-based fifo.  It allows reading and writing
 * of any block size or single bytes at a time.
 *
 * Caleb Crome
 */

#ifndef _SE_FIFO_H
#define _SE_FIFO_H
#ifdef SE_FIFO_THREAD_SAFE
#include <pthread.h>

#endif
#include "se_types.h"


/* Configure the fifo parameters here: */

/* No configuration should be necessary below here.  */
typedef struct {
    int size;
    int16 *f;
    int   r, w; // read and write indexes.
    int count;
#ifdef SE_FIFO_THREAD_SAFE
    pthread_mutex_t mutex;
    pthread_cond_t  var;
#endif
} se_fifo_t;


/* se_fifo_init(fifo, fifo_buffer, fifo_buffer_count)
 *
 * initialized the fifo.  Call with an uninitialized se_fifo_t struct,
 * and an int16 fifo_buffer that's already allocated with
 * fifo_buffer_count words.
 */
void se_fifo_init(se_fifo_t *fifo, int16 *fifo_buffer, int16 fifo_buffer_count);
void se_fifo_empty(se_fifo_t *fifo); // clear the fifo contents.
/* return the number of items in the buffer. */
int se_fifo_get_count(se_fifo_t *fifo);
/* return the number of items empty */
int se_fifo_get_count_empty(se_fifo_t *fifo);
/* return true if the fifo is currently full */
int se_fifo_is_full(se_fifo_t *fifo);
/* return true if the fifo is currently empty */
int se_fifo_is_empty(se_fifo_t *fifo);

/* stuff a new word onto the fifo */
int se_fifo_put(se_fifo_t *fifo, int16 value);
/* grab a word off the fifo */
int se_fifo_get(se_fifo_t *fifo, int16 *value);
/* stuff a block onto the fifo */
int se_fifo_put_block(se_fifo_t *fifo, int count, const int16 *value);
/* grab a block off the fifo */
int se_fifo_get_block(se_fifo_t *fifo, int count, int16 *value);
#endif
