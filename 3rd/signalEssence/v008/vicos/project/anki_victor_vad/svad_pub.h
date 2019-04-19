#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2017 Signal Essence for Anki

  Module Name  - svad_pub.h
   
  Description:

  Public interface to simple voice activity detector
**************************************************************************
*/

#ifndef __svad_pub_h
#define __svad_pub_h

typedef struct
{
    //
    // basic operating parameters
    int     BlockSize;
    float   SampleRateHz;
    float   FCutoffHz;                // 3 dB point, i.e. pole of each DC Removal filter
    float   ZetaCoef;                 // coefficient for pre-emphasis
    float   PowerAdjustmentFactor;    // conversion factor to appox dB SPL
    float   AlphaNfDown;
    float   AlphaNfUp;
    float   InitNoiseFloorPower;
    float   MinNoiseFloorPower;
    float   MaxNoiseFloorPower;
    float   AlphaTrack;
    float   AbsThreshold;
    float   StartThresholdFactor;
    float   EndThresholdFactor;
    int     HangoverCountDownStart;  // number of blocks to count down before switching to idle state

}  SVadConfig_t;

//
// set default values for SVad config object
void SVadSetDefaultConfig(SVadConfig_t *cp, int blockSize, float sampleRateHz);

#endif
#ifdef __cplusplus
}
#endif
