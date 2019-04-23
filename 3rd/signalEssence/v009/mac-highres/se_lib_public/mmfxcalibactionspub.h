#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2011 Signal Essence; All Rights Reserved
  
  Module Name  - mmfxcalibactions.h
  
  Author: Hugh McLaughlin
  
  History
  
  Jan27,11    hjm  created
  2011-11-29  ryu  split header into public/private 
  
  Description:
    Contains definitions for function calls related to calibration.
    At present, this function shares the public and private data structures
    of mmfx.  It needs to get access to data deep down to get the AEC
    coefficients and put the mmfx in modes that allow for a clean
    calibration signal out the loudspeakers and clean measurements given
    a priori knowledge that calibration is occurring.
    

**************************************************************************
*/

#ifndef ___mmfxcalibactions_pub_h
#define ___mmfxcalibactions_pub_h

#include "mmglobalsizes.h"
#include "se_types.h"
//#include "mmglobalcodes.h"

        

typedef struct 
{
    int32 BlockSize;
    int32 NumMics;
    int32 SampleRate_kHz;

    // level below which the mic is likely to be dead
    // this number is a negative number -- unless there is
    // positive coupling
    float DeadMicrophoneThreshold_dB;

    // Hz where 2 pole high pass filter kicks in
    int16 fHighPassCutHz_6dB;
    uint16 NumCalibrationBlocks;

    float pGainRelativeToMic0[MAX_MICS];

    // the rout channel to use when playing the calibration signal
    // specify -1 to use all channels (default)
    int16 RoutChan;
} MMFxCalibrationConfig_t;



void CalibrationSetDefaultConfig(MMFxCalibrationConfig_t *pConfig,
                                  int32 blockSize,
                                  int32 numMics,
                                  int32 sampleRate_kHz);


#endif
#ifdef __cplusplus
}
#endif

