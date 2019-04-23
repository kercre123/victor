/* (C) Copyright 2017 Signal Essence for Anki; All Rights Reserved

Module Name  - svad.c

Author: Hugh McLaughlin

Description:

Contains the Simple Voice Activity Detector module.

Notes:


Machine/Compiler: ANSI C

**************************************************************************/
#include "dcremove_f32.h"
#include "nfbin_f32_anki.h"
#include "sfilters.h"
#include <math.h>
#include "se_error.h"

//#include "floatmath.h"
#include "se_error.h"
#include "svad_pub.h"
#include "svad.h"
#include "vfloatlib.h"
#include <string.h>

void SVadSetDefaultConfig(SVadConfig_t *cp, int blockSize, float sampleRateHz)
{
    // fill the entire structure with zeros
    memset(cp, 0, sizeof(SVadConfig_t));

    // only set up to handle sample rate = 160000 so far
    SeAssert( (sampleRateHz == 16000.0f) );
    cp->BlockSize = blockSize;
    cp->SampleRateHz = sampleRateHz;

    // dc removal poles
    cp->FCutoffHz = 150.0f;  

    // high pass pre-emphasis coefficient
    cp->ZetaCoef = 0.8f;   // 0.2 at DC, 1.8 at highest frequency, ratio high to low = 9 or about 19 dB

    // This really needs to be adjusted at initialization depending on the signal levels.
    // If the signal level is a little on the low side then raise it up.
    // Also, the adjustment factor also needs to divide by the block size to be the correct amount.
    // Let's guess it will need a 12 dB boost, or a factor of 16 in terms of power (not amplitude)
    cp->PowerAdjustmentFactor = 16.0f;

    // noise floor estimate parameters
    cp->AlphaNfDown = 0.01f;                // avg down
    cp->AlphaNfUp = 0.015f;                 // float up
    cp->InitNoiseFloorPower = 1000.0f;      // initial value, 30 dB
    cp->MinNoiseFloorPower = 316.0f;        // 25 dB
    cp->MaxNoiseFloorPower = 10000000.0f,  // 70 dB

    // track up average down parameter
    // a time constant of about 50 milliseconds, or a drop of about 0.5 dB every block
    // Or about 5 dB in 100 ms, or 50 dB in 1 seconds.
    cp->AlphaTrack = 0.1f;   

    // absolute threshold, this is a signal level that is mandatory no matter what the background noise is
    // most humans will speak about 6 dB above the background noise, but some rooms can be very, very quiet
    cp->AbsThreshold = 100000.0f;    // about 50 dB presuming signal maps to dB signal level

    cp->StartThresholdFactor = 4.0f;   // 6 dB above noise floor
    cp->EndThresholdFactor = 4.0f;     // 6 dB above noise floor

    cp->HangoverCountDownStart = 75;   // 75 blocks equals 750 milliseonds, probably a little too short, but makes testing easier
}

// re-load runtime parameters during runtime
// from configuration object
void SVadLoadConfigParams(const SVadConfig_t *cp, SVadObject_t *sp)
{
    SeAssert((NULL != cp) && (NULL != sp));
    memmove(&(sp->Config), cp, sizeof(SVadConfig_t));
}

// initialize SVAD
void SVadInit(SVadObject_t *sp, SVadConfig_t *cp)
{
    // zero out whole object, then copy
    // params from config object to state object
    memset(sp, 0, sizeof(SVadObject_t));
    memmove(&sp->Config, cp, sizeof(SVadConfig_t));

    sp->InvBlockSize = 1.0f / (float)sp->Config.BlockSize;

    InitDcRemovalFilter_f32(&sp->DCRmvObj[0], sp->Config.FCutoffHz, 0.001f*sp->Config.SampleRateHz);   // put freq in kHz!
    InitDcRemovalFilter_f32(&sp->DCRmvObj[1], sp->Config.FCutoffHz, 0.001f*sp->Config.SampleRateHz);

    // Preemphasis Filter
    sp->PreemphState = 0.0f;

    // initialize other variables, even those these will clean up right away
    sp->BlockPower = 0.0f;
    sp->AvePowerInBlock = 0.0f;
    sp->NoiseFloor = sp->Config.InitNoiseFloorPower;
    sp->FourBlockMovingAverage = sp->AvePowerInBlock_delay1 = sp->AvePowerInBlock_delay2 = sp->AvePowerInBlock_delay3 = 0.0f;
    sp->Activity = 0;   // start with no activity
    sp->PowerTrk = 0.0f;
    sp->ActivityRatio = 0.0f;
        

    InitNoiseFloorPerBin_f32_anki(
        &sp->NoiseFloorObj,       // structure pointer to object
        sp->Config.AlphaNfDown,
        sp->Config.AlphaNfUp,  
        sp->Config.InitNoiseFloorPower,
        sp->Config.MinNoiseFloorPower,
        sp->Config.MaxNoiseFloorPower, 
        1);           // just one full band, no subbands

    // in principle it is not really necessary, but why not start clean
    sp->HangoverCountDown = sp->Config.HangoverCountDownStart;

}


// do the PreEmphasis Filter by using vector processing
// The advantage of using vector functions is to re-use optimized vector functions.
// y = x(n) + zeta*x(n-1)    // first backward difference
// retrieve previous sample and put into first postion of work buffer
// zeta should be in the range of -0.8 to -0.9

static void ApplyPreemphasis(float zetaCoeff,
    float *pPreemphState,
    const float *inPtr,
    float *outPtr,
    int   blockSize)
{
    float WorkBuf[MAX_VAD_BLOCK_SIZE];

    float *workBufPtr;

    workBufPtr = &WorkBuf[0];

    if (0.0 != zetaCoeff)
    {
        *workBufPtr = zetaCoeff *  *pPreemphState;
        *pPreemphState = *(inPtr + blockSize - 1);       // save end sample for next time as the previous Sample
        VScale_flt32(inPtr, zetaCoeff, workBufPtr + 1, blockSize - 1);
    }
    else
    {
        memset(workBufPtr, 0, blockSize * sizeof(float));
    }
    VAdd_flt32(inPtr, workBufPtr, outPtr, blockSize);
}



int DoSVad(
        SVadObject_t *sp,
        float  nfestConfidence,           // noise floor measurement confidence, set to zero if gear noise, 1.0 otherwise
        int16 *pInput_q15)                // input ptr
{
    float WorkBuf0Flt[MAX_VAD_BLOCK_SIZE];
    float WorkBuf1Flt[MAX_VAD_BLOCK_SIZE];

    // convert from 16-bit to float
    VConvert_i16_f32( pInput_q15, WorkBuf0Flt, sp->Config.BlockSize);

    // filter out DC and any low frequency noise
    // put 3 dB point at about 120 Hz, 2 poles should be sufficient.
    // 3 poles is 18 dB per octave.
    // Using rules of thumb 3 poles at 120 Hz is -9 dB at 120 Hz.
    // At 60 Hz the attenuation is 21 dB.

    // 1st pole  
    DcRemovalFilter_f32(&sp->DCRmvObj[0], WorkBuf0Flt, WorkBuf1Flt, sp->Config.BlockSize );

    // 2nd pole
    DcRemovalFilter_f32(&sp->DCRmvObj[1], WorkBuf1Flt, WorkBuf0Flt, sp->Config.BlockSize );

    // Preemphasis Filter
    ApplyPreemphasis(
        sp->Config.ZetaCoef,
        &sp->PreemphState,
        WorkBuf0Flt,
        WorkBuf1Flt,
        sp->Config.BlockSize);

    // measure power level
    sp->BlockPower = VSumPower_flt32(WorkBuf1Flt, sp->Config.BlockSize);

    // normalize to be approximately dB SPL
    sp->AvePowerInBlock = sp->Config.PowerAdjustmentFactor * sp->InvBlockSize * sp->BlockPower;

    // compute noise floor
    ComputeNoiseFloorPerBin_f32_anki(
        &sp->NoiseFloorObj,               // structure pointer to object
        nfestConfidence,                  // usually confidence in measuring a higher value
        &sp->AvePowerInBlock,             // signal power vector, actually just one element
        &sp->NoiseFloor);                 // estimated noise floor

    // average the power over 4 blocks to approximate a 40 millisecond moving average
    sp->FourBlockMovingAverage = sp->AvePowerInBlock + sp->AvePowerInBlock_delay1 + sp->AvePowerInBlock_delay2 + sp->AvePowerInBlock_delay3;
    sp->FourBlockMovingAverage *= 0.25f;  // divide average by 4
    // push the delay line
    sp->AvePowerInBlock_delay3 = sp->AvePowerInBlock_delay2;
    sp->AvePowerInBlock_delay2 = sp->AvePowerInBlock_delay1;
    sp->AvePowerInBlock_delay1 = sp->AvePowerInBlock;

    // set the end power candidate to the 4-block average.  This will eliminate bangs and ticks from persisting for a long time

    // do a track up float down measure on the end power
    TrackUpLeakDown_f32(&sp->PowerTrk, sp->FourBlockMovingAverage, sp->Config.AlphaTrack );
    
    // make decisions

    if( sp->Activity == 0 )
    {
        // look for start of activity
        float startThreshold;

        startThreshold = sp->NoiseFloor;
        if (sp->Config.AbsThreshold > startThreshold)
            startThreshold = sp->Config.AbsThreshold;

        startThreshold *= sp->Config.StartThresholdFactor;

        // use instantaneous power to determine start
        if (sp->AvePowerInBlock > startThreshold)
        {
            sp->Activity = 1;
            sp->HangoverCountDown = sp->Config.HangoverCountDownStart;
        }


        sp->ActivityRatio = sp->AvePowerInBlock / startThreshold;
    }
    else  // Activity is true, check for transition to idle
    {
        // look for end of activity
        float endThreshold;

        endThreshold = sp->NoiseFloor;
        if (sp->Config.AbsThreshold > endThreshold)
            endThreshold = sp->Config.AbsThreshold;

        endThreshold *= sp->Config.EndThresholdFactor;

        // use average and leaked down power for determining end of speech
        if (sp->PowerTrk < endThreshold)
        {
            sp->HangoverCountDown -= 1;
            if( sp->HangoverCountDown <= 0 )
                sp->Activity = 0;
        }         
        else
        {
            sp->HangoverCountDown = sp->Config.HangoverCountDownStart;
            // sp->Activity remains = 1
        }

        sp->ActivityRatio = sp->PowerTrk / endThreshold;
    }

    return sp->Activity;  // 1=active, 0=no activity
}
