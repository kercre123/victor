#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2009 Signal Essence; All Rights Reserved
  
  Module Name  - se_nr_pub.h
  
  Author: Hugh McLaughlin
  
  History
  
  Feb01,10    hjm  created
  Mar28,11    ryu  extensive renovation
  Nov28,11    ryu  split into private-public interface
  
  Description:
  SENR = signal essence noise reduction
**************************************************************************
*/

#ifndef __se_nr_pub_h
#define __se_nr_pub_h

#include "se_types.h"
#include "mmglobalcodes.h"
#include "meta_fda_pub.h"

//
// the noise reference map associates
// a noise reference channel with an optimal solution channel
#define SENR_MAX_NUM_OPT_CHANS 100  // a hack; I'm sorry
typedef struct
{
    int32 OptSolnChan;
    int32 NoiseRefChan;
} SenrNoiseRefMap_t;

//
// post-spatial filter noise reduction
// 
typedef enum
{
    SENR_FIRST_DONT_USE,
    SENR_APPLY_NOISE_REDUCTION,   // normal operation
    SENR_DISABLE_NOISE_REDUCTION, // set final suppression gain to unity 
                                  // but retain the overlap-add analysis-synthesis rigamorole
    SENR_OUTPUT_OPT_SOLN,         // copy optimal solution samples to output
    SENR_OUTPUT_NOISE_REF0,       // copy noise ref samples to output
    SENR_OUTPUT_RX_REF,           // copy rx ref samples to output
    SENR_LAST                     // this enum is always last
} SenrOutput_t;

//
// Noise Reference Gain Lookup Tables
// these enums are declared here (rather than in nrgain.h)
// because the SENR config structure refers to them.
typedef enum
{
    NRGAIN_FIRST_DONT_USE,
    NRGAIN_SNR_RANGE_6_DB,
    NRGAIN_SNR_RANGE_12_DB,
    NRGAIN_SNR_RANGE_LAST
} NrGainTable_t;

// **************************************************************
// FREQUENCY DOMAIN ECHO MODELS
// (for noise reduction algorithm)
typedef enum {
    ECHO_MODEL_FIRST_DONT_USE,

    // small wireless device, loudspeaker starts at about 700 Hz
    // microphones about 40-60 mm from loudspeaker
    ECHO_MODEL_HANDHELD,

    // directional microphone on top of screen
    // loudspeaker or speakers are multimedia loudspeakers starting at
    // about 150 Hz, mounted below the screen
    ECHO_MODEL_DESKTOP_VIDEO,

    // directional microphone on top of screen
    // loudspeaker or speakers are multimedia loudspeakers starting at
    // about 150 Hz, mounted below the screen
    // imperfectly linear echo with inconsistent convergence at high frequencies
    ECHO_MODEL_DESKTOP_VIDEO_IMPERFECT,

    //
    // similar to ECHO_MODEL_DESKTOP_VIDEO but
    // with more indirect echo
    ECHO_MODEL_CONFERENCE_ROOM,

    //
    // microphone array atop a big board
    // short AEC channel model
    ECHO_MODEL_BIG_BOARD_SHORT_AEC,

    // resembles a Puck -- speakerphone with close microphones, but beams
    // that cancel echo from the loudspeaker
    ECHO_MODEL_PUCK_LIKE,

    // boom-box music player, very loud, with
    // mic array mounted on top
    ECHO_MODEL_BOOM_BOX,

    // public address muffled -- could be a horn inside a big robot
    ECHO_MODEL_PUBLIC_ADDRESS_MUFFLED,

    // car demo
    ECHO_MODEL_CAR_DEMO,

    // zero db across band
    ECHO_MODEL_ZERO,
    
    // this value should always come last
    ECHO_MODEL_INDEX_LAST
} FdEchoModelProfile_t;

#define SENR_MAX_LEN_SEDIAG_PREFIX 10

typedef struct
{
    //
    // basic operating parameters
    int32    SampleRateHz; 
    int32    BlockSize;
    float32  ZetaEmphasisCoef; // coefficient for pre-emphasis and deemphasis
    float32  EchoSafetyFactor;  //  typical is 8.0
    float32  ResEchoWeightFactor;
    float32  ResEchoFullbandWeightFactor;
    float32  NrWeightFactor;     
    float32  NrFullbandWeightFactor; 
    float32  NfWeightFactor; 
    float32  NfFullbandWeightFactor;
    float32  ShortTermSignalToNrReverbFactor;
    float32  LongTermSignalToNrReverbFactor;
    float32  NfDeductFactor;
    float32  NrDeductFactor;
    float32  VadWeightFactor;

    // freq-domain echo modelling configuration
    FdEchoModelProfile_t EchoModelIndex;

    //
    // flags for enabling/disabling input signals in gain calculations
    // enabling/disabling a signal is equivalent to enabling/disabling
    // the corresponding gain path
    uint16                   UseNoiseRefSignal;      // 1 = use noise reference signal in calculations; 0 = ignore (treat as zero)
    uint16                   UseNoiseFloorEstim;          // 1 = use estimated noise floor; 0 = ignore (treat as 0)
    
    //
    // flags for enabling/disabling gain comparisons
    // 1 = apply computed gain
    // 0 = don't use gain (use unity gain)
    uint16                   UseNrGainPerBin;         // noise reference gain
    uint16                   UseNrGainFullband;
    
    uint16                   UseResEchoGainPerBin;    // residual echo gain
    uint16                   UseResEchoGainFullband;

    uint16                   UseNfGainPerBin;         // noise floor gain
    uint16                   UseNfGainFullband;

    uint16                   UseAdaptiveNoiseRef;

    float32                  AlphaSTrk;
    float32                  AlphaNrTrk;
    float32                  AlphaReverbTrk;
    float32                  AlphaLongTerm;
    float32                  AlphaReverbSanityDown;
    float32                  AlphaNtSanity;
    float32                  AlphaUpNtTemporalMasking;
    float32                  AlphaDownNtTemporalMasking;
    float32                  AlphaNtTemporalMaskingSanity;
    float32                  NearTemporalMaskingWeight;

    // fuzzy maps for
    // noise floor metric to weight adjustment
    float32 pNfMetricToAdjust_x[FUZZY_NUM_CURVE_PNTS];
    float32 pNfMetricToAdjustPb_y[FUZZY_NUM_CURVE_PNTS];
    float32 pNfMetricToAdjustFb_y[FUZZY_NUM_CURVE_PNTS];

    // fuzzy maps for
    // near talker confidence
    float32 pNtConfLookup_x[FUZZY_NUM_CURVE_PNTS];
    float32 pNtConfLookup_y[FUZZY_NUM_CURVE_PNTS];
    
    //
    // specify which gain lookup tables to use
    NrGainTable_t NoiseFloorGainProfile;
    NrGainTable_t ResEchoGainProfile;
    NrGainTable_t NoiseRefGainProfile;

    // Min gains for the SNR-to-gain lookup
    float32 MinGainNfPerBin;
    float32 MinGainNfFullBand;
    float32 MinGainNrPerBin;
    float32 MinGainNrFullBand;
    float32 MinGainResEchoPerBin;
    float32 MinGainResEchoFullBand;

    // gain alpha selection values
    float32 NfMinGainAlphaUp;              // noise floor gain path min alph aup
    float32 NfGainAlphaDown;               // noise floor gain path alpha down
    float32 NrMinGainAlphaUp;              // noise reference gain path min alpha up
    float32 NrMinGainAlphaDown;            // noise reference gain path min alpha down
    float32 ReMinGainAlphaUp;              // residual echo gain path min alpha up
    float32 ReMinGainAlphaDown;            // residual echo gain path min alpha down

    //
    // noise floor estimator
    float32 NfMinFullbandDb;
    float32 NfMaxFullbandDb;
    float32 NfFullbandDontCareDb;

    // 
    // sediag prefix for multiple instances of senr
    char pSeDiagPrefix[SENR_MAX_LEN_SEDIAG_PREFIX];

    int32 NumOptChans;     // number of opt solution inputs
    int32 NumNoiseRefChans; // number of noise ref inputs
    SenrNoiseRefMap_t pNoiseRefMap[SENR_MAX_NUM_OPT_CHANS];

    int32 NumRefChans;     // number of (rout) reference channels

    uint16 bypassAll;      // bypass entire noise reduction algorithm

    MetaFdaImplementation_t FdaMode;

    float32 ComfortNoiseCutoffHz;

    uint16 UseNoiseFloorMetric;

    int32 MorphoFilterLen; // length of morpho filter kernel
}  SenrConfig_t;

//
// set default values for SENR config object
void SenrSetDefaultConfig(SenrConfig_t *cp,
                          int32 blockSize,
                          float32 sampleRateHz);

#endif
#ifdef __cplusplus
}
#endif
