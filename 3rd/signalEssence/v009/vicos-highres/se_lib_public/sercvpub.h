#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2009 Signal Essence; All Rights Reserved

 Module Name  - sercvpub.h

 Author: Hugh McLaughlin

 History
     created Mar13,09

 Description:
*/

#ifndef __sercvpub_h
#define __sercvpub_h

#include "se_types.h"
#include "mmglobalcodes.h"
#include "tap_point.h"
#include "lrhpf_pub.h"
#include "se_crossover_pub.h"
//
// RcvPublicObject maintains the state of the Receive processing.
// Fields in this structure give the user read/write access to
// Receive operations.


#define SERCV_MAX_NUM_DC_RMV_POLES   4
#define SERCV_MAX_NUM_CHANNELS_IN    2

//
// default gain parameters
#define SERCV_DEFAULT_ROUT_MIN_GAIN      8192.0f
#define SERCV_DEFAULT_ROUT_RMS_MIN_LEVEL 2048.0f

// suggested headset gain parameters
#define SERCV_HEADSET_ROUT_MIN_GAIN      23166.0f
#define SERCV_HEADSET_ROUT_RMS_MIN_LEVEL 8000.0f


typedef struct
{
    int32  BlockSize_Rcv;
    int32  BlockSize_Rin;
    int32  BlockSize_Rout;
    
    float32  SampleRateHz_Rcv;
    float32  SampleRateHz_Rin;
    float32  SampleRateHz_Rout;

    int16   NoiseReductionMinGain;
    int32  FcutDcRemoval_Hz;      // cutoff freq for dc removal 
    int32  NumDcRmvPoles;

    int32   BypassEntireRcvPath;  // one bypass to rule them all (overrides individual bypasses)

    int32   BypassDcRemove;
    int32   BypassNoiseReduction;  
    int32   BypassPreClAgc;        // 0=normal, 1=bypass
    int32   BypassLrHpf;           // Bypass Level Responsive High Pass Filter, 0=normal, 1=bypass
    int32   BypassEQ;
    int32   BypassRmsLimiter;      // 0=normal, 1=bypass
    uint16   EnableLineEchoCanceller;
    int32   BypassDuplexHelp;      // ignore duplex help requests from Tx path

    //
    // noise floor estimator
    float32 NfeStepsizeDown;
    float32 NfeStepsizeUp;
    float32 NfeInitEstim;
    float32 NfeMinEstim;
    float32 NfeMaxEstim;

    //
    // POINTER to initialized, static crossoverConfig
    SeCrossoverConfig_t *pCrossoverConfig;

    LevelResponsiveFilterConfig_t  levelResponsivFilterConfig;

    // number of input channels
    // i.e. 1 = mono
    //      2 = stereo
    int32 NumRxInputChans;

    int16 pGainPerChan_q10[SERCV_MAX_NUM_CHANNELS_IN];

    //
    // full duplex help 
    float32 ExpectedTotalEchoGain;
    float32 RoutMinGain;
    float32 RoutMaxGain;
    float32 RoutRmsMinLevel;
    float32 RoutRmsMaxLevel;
} SeRcvConfig_t;

typedef struct
{
    SeRcvConfig_t Config;
    void    *VoidPrivateObjPtr;  /* pointer to private data structure */

    // tappoints for inspecting/injecting signal
    //
    // tappoints are defined in the public structure
    // because the customer may need access for customization
    //
    // one tappoint per rx chan
    TapPoint_t               *pTapPreGainWithRmsPerChan;
    TapPoint_t               *pTapPreCrossoverPerChan;  //TapFuncRoutPreCrossover;
    TapPoint_t               *pTapPostCrossoverPerChan;  //TapFuncRoutPostCrossover;

    int32 RoutNumOutputs;  // total number of output channels =  (#input channels) * (#crossover channels)
} SeRcvPublicObject_t;

typedef struct
{
    float32 TotalEchoGain;
    float32 NearTalkerCTrk;

    // pointer to Line Echo Canceller
    // ryu:  I've struggled with how to pass the LEC
    // pointer from tx to rx.  I finally put it here because this struct
    // encapsulates all data passed from tx to rx.
    void *LecPntr;

    MMFxOpMode_t RxCurrentOpMode;
} SeRcvParamsFromTxPath_t;

/* Function Prototypes */

/*=======================================================================
   Function Name:   SeRcvSetPublicDefaults()
 
   Description:     Sets defaults.

   Returns:         void
=======================================================================*/
void SeRcvSetConfigDefaults( SeRcvConfig_t *p,
                             float32 blockTimeS,
                             float32 sampleRateHz_Rin,
                             float32 sampleRateHz_Rcv,
                             float32 SampleRateHz_Rout);

void SeRcvInit( SeRcvConfig_t *pConfig, SeRcvPublicObject_t *pp);
void SeRcvDestroy(SeRcvPublicObject_t *pp);

void SeProcessRcv( 
            SeRcvPublicObject_t *pp, 
            SeRcvParamsFromTxPath_t *paramsFromTxPath,
            const int16 *rinPtr,
            int16 *routPtr );


/* ====================================================================
    Get pointers to singleton config and state objects
==================================================================== */
SeRcvPublicObject_t* SeRcvGetSingletonPubObject(void);

#endif

#ifdef __cplusplus
}
#endif

