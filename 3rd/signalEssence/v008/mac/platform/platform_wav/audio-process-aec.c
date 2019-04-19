#include "audio-process.h"
#include "mmif.h"
#include "vfixlib.h"
#include <stdio.h>
#include <math.h>
#include "mmfxpub.h"
#include "se_timer.h"

#if defined ( _MSC_VER )
#define __func__ __FUNCTION__
#endif

audio_process_error_t audio_process_init(platform_data_t *platform) 
{
    MMIfInit(platform->ref_delay_sec, NULL);
    return AP_ERR_OK;
}

void audio_process_destroy(void)
{
    MMIfDestroy();
}

void spin(long int s) {
    volatile int i;
    volatile long int ss = s;
    while(ss > 0) {
        for (i = 0; i < 100; i++);
        ss--;
    }
}


#define AEC
int ramp = 0;

audio_process_error_t audio_process_tx(platform_data_t *platform, int16 *SinBuf, int16 *SoutBuf, int16 *RefBuf) {
    se_timer_start("audio_process", 0);
#ifdef SPIN
    spin(10000);
#endif
#ifdef AEC  
    UNUSED(platform);
    MMIfProcessMicrophones( RefBuf, SinBuf, SoutBuf);
#endif
    se_timer_stop("audio_process");
    return AP_ERR_OK;
}

audio_process_error_t audio_process_rx(platform_data_t *platform, int16 *RinBuf, int16 *RoutBuf) {
    se_timer_start("audio_process", 0);
    UNUSED(platform);
    RcvIfProcessReceivePath(RinBuf, RoutBuf);
    se_timer_stop("audio_process");
    return AP_ERR_OK;
}

#include "mmfxpub.h"
#include <stdio.h>


void NrRedirectInputToWav(int16* pRxRef, int16* pOptSoln, int16* pNoiseRef, int blockSize)
{
    UNUSED(pRxRef);
    UNUSED(pOptSoln);
    UNUSED(pNoiseRef);
    UNUSED(blockSize);
}


void NrRedirectOutputToWav(int16* pOut, int blockSize)
{
    UNUSED(    pOut);
    UNUSED(blockSize);
}

