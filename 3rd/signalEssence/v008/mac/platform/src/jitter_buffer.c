#include <stdlib.h>
#include <string.h>
#include "jitter_buffer.h"
#include "se_error.h"
#include "scratch_mem_pub.h"
#include "vfixlib.h"


static double b[] = {2.413590e-04, 4.827181e-04, 2.413590e-04};
static double a[] = {-1.955578e+00, 9.565437e-01};
#define a_len  (sizeof(a)/sizeof(a[0]))

void jb_pop_(jitter_buffer_t *jb, void *frames, int frame_count);

int jb_used_frames(jitter_buffer_t *jb)
{
    int frames = (jb->head_frame - jb->tail_frame);
    if (frames < 0)
        frames += jb->buffer_size_frames;
    SeAssert(frames >= 0);
    return frames;
}
int jb_free_frames(jitter_buffer_t *jb)
{
    return (jb->buffer_size_frames - jb_used_frames(jb));
}

static int jb_byte_offset(jitter_buffer_t *jb, int frame_count)
{
    return jb->channels * jb->sample_size_bytes * frame_count;
}

void jb_init(jitter_buffer_t *jb, int channels, int sample_size_bytes, int buffer_size_frames)
{
    memset(jb, 0, sizeof(*jb));
    jb->sample_size_bytes = sample_size_bytes;
    jb->channels = channels;
    jb->buffer_size_frames = buffer_size_frames;
    jb->buffer_size_bytes = channels * buffer_size_frames * sample_size_bytes;
    jb->buffer = calloc(jb->buffer_size_bytes, 1);
    jb->target = buffer_size_frames / 2;
    jb->p = 0.05;
    // jb->i = 0.00001;
    fiir_init(&jb->push_filter, a_len, b, a);
    fiir_init(&jb->pop_filter, a_len, b, a);
}
void jb_destroy(jitter_buffer_t *jb)
{
    free (jb->buffer);
    memset(jb, 0, sizeof(*jb));
}
void jb_push_(jitter_buffer_t *jb, const void *frames, int frame_count, int ignore_calculations)
{
    if (!ignore_calculations) {
        float err;
        float y;
        jb->push_filter_y = fiir_run(&jb->push_filter, jb_used_frames(jb));
        err = jb->target - jb->push_filter_y;
        jb->i_state += err * jb->i;
        y = err * jb->p + jb->i_state;
        jb->err = y;
        if (y > 1) {
            jb->added_samples ++;
            jb_push_(jb, frames, 1, 1);
        } else if (y < -1) {
            int32 junk_frame[100];
            jb->deleted_samples++;
            jb_pop_(jb, junk_frame, 1);
        }
    }
    
    if (frame_count > jb_free_frames(jb)) {
        jb->failed_pushes++;
    } else {
        int start = (jb->head_frame % jb->buffer_size_frames);
        int len = frame_count;
        int len2 = 0;
        int bytes, bytes2 = 0;
        int total_bytes;
        if (start+frame_count > jb->buffer_size_frames) {
            len = jb->buffer_size_frames - start;
            len2 = frame_count - len;
            SeAssert(len+len2 == frame_count);
        }
        bytes = jb_byte_offset(jb, len);
        SeAssert(bytes > 0);
        memcpy(&((char *)jb->buffer)[jb_byte_offset(jb, start)], frames, bytes);
        if (len2 > 0) {
            bytes2 = jb_byte_offset(jb, len2);
            memcpy((char *)jb->buffer, ((char *)frames)+bytes, bytes2);
        }
        jb->head_frame += frame_count;
        jb->successful_pushes++;
        total_bytes = bytes+bytes2;
        {
            int x = jb_byte_offset(jb, frame_count);
            SeAssert(total_bytes == x);
        }
    }
}
void jb_push(jitter_buffer_t *jb, const void *frames, int frame_count)
{
#if !defined(_MSC_VER)
    pthread_mutex_lock(&jb->lock);
#endif
    jb_push_(jb, frames, frame_count, 0);
#if !defined(_MSC_VER)
    pthread_mutex_unlock(&jb->lock);
#endif
}
void jb_pop_(jitter_buffer_t *jb, void *frames, int frame_count)
{
    jb->pop_filter_y = fiir_run(&jb->push_filter, jb_used_frames(jb));

    if (frame_count > jb_used_frames(jb)) {
        int nbytes = jb_byte_offset(jb, frame_count);
        memset(frames, 0, nbytes);
        jb->failed_pops++;
    } else {
        int start = (jb->tail_frame % jb->buffer_size_frames);
        int len = frame_count;
        int len2 = 0;
        int bytes, bytes2;

        if (start+len > jb->buffer_size_frames) {
            len = jb->buffer_size_frames - start;
            len2 = frame_count - len;
        }
        bytes=jb_byte_offset(jb, len);
        memcpy(frames, &((char *)jb->buffer)[jb_byte_offset(jb, start)], bytes);
        if (len2 > 0) {
            bytes2 = jb_byte_offset(jb, len2);
            memcpy(&((char *)frames)[bytes]  , jb->buffer, bytes2);
        }
        jb->tail_frame += frame_count;
        jb->successful_pops++;
    }
}

void jb_pop(jitter_buffer_t *jb, void *frames, int frame_count)
{
#if !defined(_MSC_VER)
    pthread_mutex_lock(&jb->lock);
#endif
    jb_pop_(jb, frames, frame_count);
#if !defined(_MSC_VER)
    pthread_mutex_unlock(&jb->lock);
#endif
}


void jb_push_i16(jitter_buffer_t *jb, const int16 *frames, int frame_count)
{
    switch (jb->sample_size_bytes) {
    case 2:
        jb_push(jb, frames, frame_count);
        break;
    case 4:
        {
            int32 *i32_frames = ScratchGetBufX(frame_count * jb->channels * 4);
            int left_shift = 16;
            VShiftLtRnd_i16_i32(frames, i32_frames, left_shift, frame_count * jb->channels);
            jb_push(jb, i32_frames, frame_count);
            ScratchReleaseBufX(i32_frames);
        }
        break;
    default:
        SeAssert(0);
    }
}
void jb_pop_i16 (jitter_buffer_t *jb, int16 *frames, int frame_count)
{
    switch(jb->sample_size_bytes) {
    case 2:
        jb_pop(jb, frames, frame_count);
        break;
    case 4:
        {
            int32 *i32_frames = ScratchGetBufX(frame_count * jb->channels * 4);
            int right_shift = 16;
            jb_pop(jb, i32_frames, frame_count);
            VShiftRtRnd_i32_i16(i32_frames, frames, right_shift, frame_count * jb->channels);
            ScratchReleaseBufX(i32_frames);
        }
    }
}
void jb_push_i32(jitter_buffer_t *jb, const int32 *frames, int frame_count);
void jb_pop_i32 (jitter_buffer_t *jb, int32 *frames, int frame_count);
