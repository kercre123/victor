/* (C) Copyright 2017 Signal Essence; Modified for Anki. All Rights Reserved

  Author: Hugh McLaughlin

  Description:

    Noise Floor per Bin Estimator.
    The input power is expected to be float32
    It is presumed that the maximum noise floor ever will be 70 dB max per bin.
    Therefore there should be almost 6 bits of headroom vs. a rail to rail sine
    wave which would be about 87.3 dB.  

  History:

    Dec 15, 2017   hjm   created from Signal Essence nfbin_f32.c 

----------------------------------------------------------------------- */

#include "nfbin_f32_anki.h"
#include <stdlib.h>
#include "vfloatlib.h"
#include "se_error.h"
#include <string.h>

/* =================================================================
   initialize (floating point) noise floor estimator
==================================================================== */
void InitNoiseFloorPerBin_f32_anki(
        NfBin_f32_t *np,
        float downStepSize,
        float floatUpRate,
        float initNf,
        float minNf,   // min noise floor, linear power
        float maxNf,   // max noise floor, linear power
        int numBins )
{
    float maxNoiseFloor;

    SeAssert( numBins < FD_MAX_GROUPED_BINS);

    // hygiene, clear all memory, which will also clear the floating point state
    memset(np, 0, sizeof(NfBin_f32_t));

    np->KDown = downStepSize;
    np->KFloatUp = floatUpRate;
    np->InitNf = initNf;

    // since the NfBin structure maintains its own 

    np->NumBins = numBins;

    maxNoiseFloor = (32767.0f*32767.0f);
    SeAssert(initNf < maxNoiseFloor);
    SeAssert( maxNf < maxNoiseFloor);

    VFill_flt32(np->NfBinState,
        initNf,
        numBins);

    np->MaxNf = maxNf;
    np->MinNf = minNf;

}



/*=================================================================
   Function Name:    ComputeNoiseFloorPerBin()

   Description:

       This is a  noise floor estimator for estimating the persistent
       noise in each bin in a vector of bin powers.  The theory of operation is that 
       the noise floor measure responds slowly to levels which are higher than the 
       current estimate while responding to new lower levels very quickly.  
       The estimate goes up at a steady rate determined by the "Up" factor, 
       but the estimate goes down in proportion to the error between the new 
       lower value and the current state.

   IN
   *np - noise floor o

   Returns:          void
====================================================================*/
void ComputeNoiseFloorPerBin_f32_anki( NfBin_f32_t *np,        // structure pointer to object
                                  float conf,                  // confidence in measuring a higher value
                                  float *xPwrPtr,              // signal power vector
                                  float *nfBinPtr )            // estimated noise floor
{

    float C_X_KFloatUp;  // C * KFloatUp
    float C_X_KDown;  // C * KFloatDown
    float delta;      // change to noise floor

    int   i;         // loop counter
    int   numBins;
    float *nfBinStatePtr;
    
    SeAssert((conf>=0.0f) && (conf<=1.0f));

    numBins  = np->NumBins;
    nfBinStatePtr = np->NfBinState;

    //
    // scale the up/down constants by the measurement confidence
    // compute this outside the loop
    C_X_KFloatUp = np->KFloatUp * conf;
    C_X_KDown = np->KDown * conf;

    for( i=0; i<numBins; i++ )
    {
        float32 nfEst;
        float32  x;
        x = xPwrPtr[i];

        if( x > np->MaxNf )
        {
            x = np->MaxNf;
        }

        if( x < np->MinNf ) 
        {
            x = np->MinNf;
        }

        nfEst = nfBinStatePtr[i];
        if( x > nfEst )
        {
            //
            // ADJUST UP

            // nfEst += (nfEst * C * np->KFloatUp);
            delta = C_X_KFloatUp * nfEst;
        }
        else
        {
            float32 diff;       // temporary value    
            // 
            // ADJUST DOWN

            // np->NfEst += C*np->KDown*(x - np->NfEst);
            diff = x - nfEst;
            SeAssert(diff<=0.0);

            delta = C_X_KDown * diff;
        }
        nfEst += delta;

        //
        // update noise estimate state vector
        // and output
        nfBinStatePtr[i] = nfEst;
        nfBinPtr[i] = nfBinStatePtr[i];
    }
}
