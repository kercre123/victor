#ifndef _AUDIO_PROCESS_H
#define _AUDIO_PROCESS_H
#include "se_types.h"

typedef struct {
    int num_mics;
    float32 ref_delay_sec;
    int enable_sout_delay;
    int enable_rin_delay;
    float32 runtime_last_block; //  the time it took the last block to run, in milliseconds
    void *private; // private stuff used for any purpose that the platform likes.

    int blocksize_rin;
    int blocksize_sout;
    int num_chans_rin;
    int num_chans_sout;

    int num_overrun_blocks; // number of blocks which took too long
} platform_data_t;
typedef enum {
    AP_ERR_OK = 0,
    AP_ERR_NOT_ENOUGH_MICROPHONES = 1
} audio_process_error_t;
/* audio_process_init 
 * initialize the platform data.
 * return AP_ERR_OK on success, or 
 * one of the AP_ERR_x types on error.
 */
audio_process_error_t audio_process_init(platform_data_t *platform) ;
void audio_process_destroy(void);
audio_process_error_t audio_process_rx(platform_data_t *platform, int16 *RinBuf, int16 *RoutBuf);
audio_process_error_t audio_process_tx(platform_data_t *platform, int16 *SinBuf, int16 *SoutBuf, int16 *RefBuf);
#endif
