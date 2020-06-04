#include "se_fifo.h"
#include <string.h>

#ifndef SE_ASSERT
#define SE_ASSERT(x) if ((!x)) while (1);
#endif

void se_fifo_init(se_fifo_t *fifo, int16 *fifo_buffer, int16 fifo_buffer_count)
{
  int i;
  fifo->f = fifo_buffer;
  fifo->size = fifo_buffer_count-1;
  for (i = 0; i < fifo->size+1; i++) {
    fifo->f[i] = 0;
  }
  fifo->r = 0;
  fifo->w = 0;
  fifo->count = 0;
#ifdef SE_FIFO_THREAD_SAFE
  pthread_mutex_init(&fifo->mutex, 0);
  pthread_cond_init(&fifo->var, 0);
#endif
}

void se_fifo_empty(se_fifo_t *fifo)
{
#ifdef SE_FIFO_THREAD_SAFE
    pthread_mutex_lock(&fifo->mutex);
#endif
  int i;
  for (i = 0; i < fifo->size+1; i++) {
    fifo->f[i] = 0;
  }
  fifo->r = 0;
  fifo->w = 0;
  fifo->count = 0;
#ifdef SE_FIFO_THREAD_SAFE
    pthread_cond_signal(&fifo->var);
    pthread_mutex_unlock(&fifo->mutex);
#endif
}

int se_fifo_get_count(se_fifo_t *fifo) {
    int result = fifo->count;
    return result;
}

int se_fifo_get_count_empty(se_fifo_t *fifo) {
    return fifo->size - fifo->count;
}

int se_fifo_is_full(se_fifo_t *fifo) {
    return (fifo->count == fifo->size);
}
int se_fifo_is_empty(se_fifo_t *fifo) {
    return (fifo->count == 0);
}

/* fifo_put
 * 
 * put 1 word onto the fifo.  On Success, return 1.  On failure (full fifo), 
 * return 0
 */
int se_fifo_put(se_fifo_t *fifo, int16 value) {
    int result;
#ifdef SE_FIFO_THREAD_SAFE
    pthread_mutex_lock(&fifo->mutex);
#endif
    if (fifo->count >= fifo->size) {
        result = 1; // error
    } else {
        fifo->f[fifo->w] = value;
        fifo->w++;
        if (fifo->w >  fifo->size) 
            fifo->w = 0;
        fifo->count++;
        result = 0;
    }
#ifdef SE_FIFO_THREAD_SAFE
    pthread_cond_signal(&fifo->var);
    pthread_mutex_unlock(&fifo->mutex);
#endif
    return result;
}

/* fifo_get
 *
 * get one word from the fifo.  if it's already empty, return 1.  else
 * return 0 and fill the 'value' parameter.
 */
int se_fifo_get(se_fifo_t *fifo, int16 *value) {
    int result = 0;
#ifdef SE_FIFO_THREAD_SAFE
    pthread_mutex_lock(&fifo->mutex);
#endif
  if (fifo->r == fifo->w) {
	*value = 0;
	result = 1; // error
  } else {
	*value = fifo->f[fifo->r];
	fifo->r++;
	if (fifo->r > fifo->size) 
	  fifo->r = 0;
	fifo->count--;
	result = 0; // okay
  }
#ifdef SE_FIFO_THREAD_SAFE
  pthread_cond_signal(&fifo->var);
  pthread_mutex_unlock(&fifo->mutex);
#endif
  return result;
}


int se_fifo_put_block(se_fifo_t *fifo, int count, const int16 *value)
{
#ifdef SE_FIFO_THREAD_SAFE
    pthread_mutex_lock(&fifo->mutex);
#endif
  // Put a block of data into the fifo.
    int result = 0;
  int start1 = fifo->w;
  int end1  = start1 + count - 1;
  int count1 = count;
  const int start2 = 0;
  int count2 = 0;

  if (count > fifo->size - fifo->count)  {
      result = 1; // error
  } else {
  
      if (end1 > fifo->size) {
          // this is going to go off the end of the buffer
          count1  = fifo->size - fifo->w + 1;
          count2  = count - count1;
      }
      memcpy   (&(fifo->f[start1]), value       , count1 * sizeof(*value));
      if (count2 > 0) {
          memcpy (&(fifo->f[start2]), value+count1, count2 * sizeof(*value));
      }
      fifo->count += count;
      fifo->w += count;
      if (fifo->w > fifo->size) 
          fifo->w = fifo->w - fifo->size - 1;
  }
#ifdef SE_FIFO_THREAD_SAFE
  pthread_cond_signal(&fifo->var);
  pthread_mutex_unlock(&fifo->mutex);
#endif
  return result;
}
int se_fifo_get_block(se_fifo_t *fifo, int count, int16 *value)
{
#ifdef SE_FIFO_THREAD_SAFE
    pthread_mutex_lock(&fifo->mutex);
#endif
    int result = 0;
  int start1 = fifo->r;
  int count1 = count;
  int end1 = start1+count-1;
  const int start2 = 0;
  int count2 = 0;
  if (count > fifo->count) {
      result = 1;
  } else {
      if (end1 > fifo->size) {
          count1 = fifo->size - fifo->r + 1;
          count2 = count - count1;
      }
      memcpy  (value,        &(fifo->f[start1]), count1 * sizeof(*value));
      if (count2 > 0) {
          memcpy(value+count1, &(fifo->f[start2]), count2 * sizeof(*value));
      }
      fifo->count -= count;
      fifo->r += count;
      if (fifo->r > fifo->size)
          fifo->r = fifo->r - fifo->size - 1;
  }
#ifdef SE_FIFO_THREAD_SAFE
  pthread_cond_signal(&fifo->var);
  pthread_mutex_unlock(&fifo->mutex);
#endif
  return result;
}

#ifdef SE_FIFO_TEST
// Compile command
// gcc -o se_fifo_test se_fifo.c -DSE_FIFO_TEST -Dfifo->size=30 -Dint16=int
#include <stdio.h>
#include <assert.h>
se_fifo_t f;
int main(int argc, char *argv) {
  int i;
  int errors = 0;
  int16 v;
  se_fifo_init(&f);
 for (i = 0; i < fifo->size; i++) {
	errors += se_fifo_put(&f, i);
  }
  assert(errors == 0);
  errors += se_fifo_put(&f, i);
  assert(errors == 1);

  errors = 0;
  for (i = 0; i < fifo->size; i++) {
	errors += se_fifo_get(&f, &v);
	assert(errors == 0);
	assert(v == i);
  }
  errors += se_fifo_get(&f, &v);
  assert(errors == 1);
  printf ("SE_FIFO tests pass\n");
  return 0;
}
#endif
