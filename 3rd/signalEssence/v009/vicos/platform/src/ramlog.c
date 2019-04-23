#include "ramlog.h"
#include <string.h>
#include <malloc.h>
#include <time.h>

ramlog_error_t ramlog_init(ramlog_t *log, int count)
{
    memset(log, 0, sizeof(*log));
    log->n_elements = count;
    clock_gettime(CLOCK_REALTIME, &log->time_start);
    if ( (log->times = malloc(count * sizeof(log->times[0]))) == NULL) {
        log->n_elements = 0;
        return RAMLOG_NOMEM;
    }
    if ( (log->data = malloc(count * sizeof(log->data[0]))) == NULL) {
        log->n_elements = 0;
        free(log->times);
        return RAMLOG_NOMEM;
    } 
    log->current= 0;
    log->ptimes = &log->times[0];
    log->pdata  = &log->data[0];

    sem_init(&log->sem, 0, 1);
    return RAMLOG_OK;
}
ramlog_error_t ramlog_deinit(ramlog_t *log) {
    free(log->data);
    free(log->times);
    return RAMLOG_OK;
}

ramlog_error_t ramlog_log(ramlog_t *log, const ramlog_data_t data)
{
    sem_wait(&log->sem);
    memcpy       (log->pdata, &data, sizeof(data));
    clock_gettime(CLOCK_REALTIME, log->ptimes);
    log->ptimes++;
    log->pdata++;
    log->current++;
    if (log->current >= log->n_elements) {
        log->has_looped = 1;
        log->current = 0;
        log->ptimes = &log->times[0];
        log->pdata  = &log->data[0];
    }
    sem_post(&log->sem);
    return RAMLOG_OK;
}
        
ramlog_error_t ramlog_dump(ramlog_t *log, ramlog_callback callback) {
    int i;
    int start, stop;
    int start2, stop2;
    if (!log->has_looped) {
        start = 0;
        stop  = log->current;
        start2 = 0;
        stop2  = 0;
    } else {
        start = log->current;
        stop  = log->n_elements;
        start2= 0;
        stop2 = log->current;
    }
    long long unsigned int time_start = log->time_start.tv_nsec  + log->time_start.tv_sec * 1000000000;
    for (i = start; i < stop; i++) {
        long long unsigned int time_ns = log->times[i].tv_nsec + log->times[i].tv_sec * 1000000000;
        time_ns -= time_start;
        callback(time_ns, &log->data[i]);
    }
    for (i = start2; i < stop2; i++) {
        long long unsigned int time_ns = log->times[i].tv_nsec + log->times[i].tv_sec * 1000000000;
        time_ns -= time_start;
        callback(time_ns, &log->data[i]);
    }

    return RAMLOG_OK;
}
