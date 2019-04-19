#include "fiir.h"
#if !defined(_MSC_VER)
#include <pthread.h>
#endif
#include "mmglobalcodes.h"

typedef struct {
    int sample_size_bytes;
    int channels;
    int buffer_size_frames;
    void *buffer;

    int buffer_size_bytes;
    int head_frame, tail_frame;
    int failed_pushes, failed_pops;
    int successful_pushes, successful_pops;

    fiir_t push_filter, pop_filter;
    float  push_filter_y, pop_filter_y;
    double push_last, pop_last;

    float target, i_state, p, i; // pid level controll variables.
    int added_samples;
    int deleted_samples;
    float err;

#if !defined(_MSC_VER)
    pthread_mutex_t lock;
#endif
} jitter_buffer_t;

void jb_init(jitter_buffer_t *jb, int channels, int sample_size_bytes, int buffer_size_frames);
void jb_destroy(jitter_buffer_t *jb);

// These functions work with the native sample size as defined in the jb_init function.
void jb_push(jitter_buffer_t *jb, const void *frames, int frame_count);
void jb_pop (jitter_buffer_t *jb, void *frames, int frame_count);

int jb_used_frames(jitter_buffer_t *jb); // number of frames used
int jb_free_frames(jitter_buffer_t *jb); // number of frames available

// These functions take int16 arguments, but store internally in whatever format was in jb_init.
void jb_push_i16(jitter_buffer_t *jb, const int16 *frames, int frame_count);
void jb_pop_i16 (jitter_buffer_t *jb, int16 *frames, int frame_count);
void jb_push_i32(jitter_buffer_t *jb, const int32 *frames, int frame_count);
void jb_pop_i32 (jitter_buffer_t *jb, int32 *frames, int frame_count);
