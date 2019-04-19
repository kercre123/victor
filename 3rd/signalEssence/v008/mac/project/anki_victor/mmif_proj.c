/**************************************************************************
 (C) Copyright 2016 SignalEssence; All Rights Reserved

  Module Name  - mmif_proj.c
  
   Description:
      Contains the Multi-Microphone Audio Algorithm Interface 
      layer as well as the SE Receive Processor interface.

  Machine/Compiler: ANSI C, Visual C

**************************************************************************/
#define AUTOGEN_CONFIG_DEFINES_ONLY
#include "aec_pbfd_pub.h"
#include "mmfxpub.h"
#include "mmif.h"
#include "mmif_proj.h"
#include "policy_actions.h"
#include "sercvpub.h"
#include "spatialfilterconfig.h"
#include <se_error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "autogen_config.h"

// actual number of mics in use for this project
extern int MMIfNumMics;
extern int MMIfNumSearchBeams;

// MMIF version ID
// Use MMIfGetVersionString() to retrieve this string
//
char MMIF_VERSION_ID[] = "anki_victor_008";

//
// length of acoustic echo canceller channel model,
#define AEC_LEN_CHAN_MODEL_SEC 0.0  // aec chan model length

extern MicrophoneLocation_mm microphoneLocations_mm[];
extern FdBeamSpecObj_t FdBeamArray[];
extern int16 SubbandWeightsQ10[];
extern int16 BeamWeights[];
extern float DirectivityComp[][MAX_NUM_FD_SUBBANDS];

// reference spatial filter specifiers in spatialfilterconfig.c
extern SpatialFilterSpec_t     SpatialFilterSpec;
extern LocationToBeamMapping_t LocationToBeamMapping;

#define MMIF_LEN_SCRATCH_X (1830 * 1000)
#define MMIF_LEN_SCRATCH_H (2    * 1000)
#define MMIF_LEN_SCRATCH_S (2000 * 1000)
char pScratchH[MMIF_LEN_SCRATCH_H];
char pScratchX[MMIF_LEN_SCRATCH_X];
char pScratchS[MMIF_LEN_SCRATCH_S];

//
// keep track of whether MMIf layer is initialized
static int isMMIfInitialized = 0;

// singleton state struct
typedef struct
{
    SenrConfig_t senrConfigSpeechReco;
    SenrConfig_t senrConfigTelecom;
    
    MMIfOpMode_t requestedOperatingMode;
    MMIfOpMode_t currOperatingMode;

    // sediag ids
    int idFdsPowerCompareValue;
    int idFdsConfidenceState;
    
    int numFdsBeams;
} MMIfState_t;
static MMIfState_t MMIfState;

SEDiagEnumDescriptor_t MMIfOpModeDescriptor[] = {
    DIAG_ENUM_DECLARE(    MMIF_FIRST_DONT_USE,    "do not use"),
    DIAG_ENUM_DECLARE(    MMIF_SPEECH_RECO_MODE,  "speech recognizer mode"),
    DIAG_ENUM_DECLARE(    MMIF_TELECOM_MODE,      "human-to-human communication"),
    DIAG_ENUM_DECLARE(    MMIF_LAST,              "do not use"),
    DIAG_ENUM_END
};

//
// senr mode configuration for various operating modes
///////////////////////////////////////////////////
static void ConfigSenrSpeechReco(SenrConfig_t *pCfg)
{
    SenrSetDefaultConfig(pCfg, 
                         MMIfGetBlockSize(PORT_SEND_WORKING),
                         MMIfGetSampleRateHz(PORT_SEND_WORKING));

    //
    // no echo cancellation for anki victor

    // in fact, no SENR at all (in order to save cycles)
    pCfg->bypassAll = 1;
}

static void ConfigSenrTelecom(SenrConfig_t *pCfg)
{
    SenrSetDefaultConfig(pCfg, 
                         MMIfGetBlockSize(PORT_SEND_WORKING),
                         MMIfGetSampleRateHz(PORT_SEND_WORKING));

    //
    // no echo cancellation for anki victor

    // in fact, no SENR at all (in order to save cycles)
    pCfg->bypassAll = 1;
}

//
// initialize project-specific state object
static void InitMMIfState(MMIfState_t *pState)
{
    memset(pState, 0, sizeof(*pState));

    //
    // configure SENR configuration objects for various modes
    ConfigSenrSpeechReco( &pState->senrConfigSpeechReco);
    ConfigSenrTelecom(    &pState->senrConfigTelecom);

    //
    // initial 
    pState->requestedOperatingMode = MMIF_SPEECH_RECO_MODE;    // initial operating mode
    pState->currOperatingMode      = MMIF_FIRST_DONT_USE;

    //
    // get sediag ids
    pState->idFdsPowerCompareValue = SEDiagGetIndex("fdsearch_power_compare_value");
    SeAssert(pState->idFdsPowerCompareValue >= 0);

    pState->idFdsConfidenceState = SEDiagGetIndex("fdsearch_confidence_state");
    SeAssert(pState->idFdsConfidenceState >= 0);

    pState->numFdsBeams = (int)SEDiagGetUInt16(SEDiagGetIndex("fdsearch_num_beams_to_search"));
    SeAssert(pState->numFdsBeams > 0);
}

//
// publish project-specific se diagnostics
static void PublishDiags(MMIfState_t *pState)
{
    SeAssert(SEDiagIsInitialized()==1);
    SEDiagNewEnum("mmif_requested_operating_mode", (enum_t *)&(pState->requestedOperatingMode), SE_DIAG_RW, NULL, "project-specific operating mode", MMIfOpModeDescriptor);
    MMIfPublishIdString(MMIF_VERSION_ID);
}

/*
functions to set/get location search thresholds
during gear noise. 

Raise the threshold just before the onset of gear noise. The default value is 800.0.

If you can't do this, then we also need to set fdsearch_confidence_state to 0
at the onset of noise.

Return the threshold to the default when the gear noise stops.
*/
void MMIfSetGearNoiseThreshold(float32 thresh)
{
    MMIfState_t *pState = &MMIfState;
    SeAssert(thresh >= 0.0f);
    SEDiagSetFloat32(pState->idFdsPowerCompareValue, thresh);
}

float32 MMIfGetGearNoiseThreshold(void)
{
    MMIfState_t *pState = &MMIfState;
    float32 thresh;
    thresh = SEDiagGetFloat32(pState->idFdsPowerCompareValue);
    return thresh;
}

/*
reset location search confidence state
*/
void MMIfResetLocationSearch(void)
{
    MMIfState_t *pState = &MMIfState;
    int n;
    SeAssert(pState->numFdsBeams > 0);
    for (n=0;n<pState->numFdsBeams;n++)
    {
        SEDiagSetFloat32N(pState->idFdsConfidenceState, 0.0f, n);
    }
}

/*=======================================================================

  Function Name:   RcvIfInit()

  Description:     Initializes Receive Path signal processor 
  =======================================================================*/
static void RcvIfInit(float blockTimeS, float rxSampleRateHz)
{
    SeRcvPublicObject_t* pSeRcvPubObj = SeRcvGetSingletonPubObject();
    SeRcvConfig_t seRcvConfig;
    SeRcvConfig_t* pSeRcvConfig = &seRcvConfig;

    SeRcvSetConfigDefaults( pSeRcvConfig,
                            blockTimeS,
                            rxSampleRateHz,
                            rxSampleRateHz,
                            rxSampleRateHz);
    
    pSeRcvConfig->pCrossoverConfig = &crossoverConfig;

    SeRcvInit( pSeRcvConfig, pSeRcvPubObj);
}

//
// update operating mode 
static void UpdateOpMode(MMIfState_t *pState)
{
    MMFxPublicObject_t* pMMFxPubObj;
    pMMFxPubObj = MMFxGetSingletonPubObject();        

    if (pState->requestedOperatingMode != pState->currOperatingMode) 
    {
        switch(pState->requestedOperatingMode)
        {
        case MMIF_TELECOM_MODE:
            MMFxReconfigNoiseReduction(pMMFxPubObj,&pState->senrConfigTelecom);            
            pState->currOperatingMode = pState->requestedOperatingMode;
            break;

        case MMIF_SPEECH_RECO_MODE:
            MMIfBypassRxFullDuplexHelp(1);  // no full duplex help
            MMFxReconfigNoiseReduction(pMMFxPubObj,&pState->senrConfigSpeechReco);            
            pState->currOperatingMode = pState->requestedOperatingMode;
            break;

        default:
            // do nothing
            // normally I'd assert, but since we are publishing operatingMode via sediag,
            // it seems too easy to enter an invalid value and crash the whole thing
            ; 
        }
    }
}

//
// configure AEC
static void ConfigAec(MMFxConfig_t* pMMFxConfig, float32 sampleRateHz, float32 lenChanModelSec)
{
    MultiAecConfig_t* pMultiAecConfig = MMFxGetMultiAecConfig(pMMFxConfig);
    int n;

    SeAssert(lenChanModelSec == 0.0);
    SeAssert(sampleRateHz);
    /*
      anki doesnt have echo cancellation
    */
    for (n=0;n<pMMFxConfig->NumMics;n++)
    {
        pMultiAecConfig->pBypassCancelPerAec[n] = 1;
        pMultiAecConfig->pBypassUpdatePerAec[n] = 1;
    }
}

/*=======================================================================*/
//
//  Function Name:   MMIfInit()
//
//  Description:     Initializes everything.  Presumes that the proper
//                   memory has been allocated by allocating memory
//                   for the MMFxObject_t.
/*=======================================================================*/
void MMIfInit(float32 additionalRefDelaySec, void *pArgs)
{
    uint16 numMics = (uint16)MMIfNumMics;   

    float32 sinSampleRateHz = 16000.0f;
    float32 sendSampleRateHz = 16000.0f;
    float32 soutSampleRateHz = 16000.0f;
    float32 rxSampleRateHz = 16000.0f;
    float32 blockTimeS = 0.01f;
    uint16 ib;   // beam index
    MMFxConfig_t MMFxConfig;
    MMFxConfig_t* pMMFxConfig = &MMFxConfig;
    MMFxPublicObject_t* pMMFxPubObj;
    UNUSED(pArgs);

    SeAssert( numMics <= MAX_MICS );

    // ryu 2012.10.01
    // if we're already initialized, then skip initialization
    if (1==isMMIfInitialized)
        return;

    // get pointers to mmfx's singleton instances
    pMMFxPubObj = MMFxGetSingletonPubObject();

    // initialize fields in MMFxConfig
    // MMFxConfig_t defined in mmfxpub.h
    //////////////////////////////////////////////////
    MMFxSetDefaultConfig( pMMFxConfig,
                          numMics,
                          blockTimeS,
                          sendSampleRateHz, // Send, processing rate
                          sinSampleRateHz,  // Sin, rate from analog codec
                          soutSampleRateHz, // Sout, rate outgoing
                          sinSampleRateHz, // Refin, same as mics
                          rxSampleRateHz,   // Rcv,
                          rxSampleRateHz,   // Rin,
                          rxSampleRateHz    // Rout
        );

    // Configure ref signal processing
    {
        int additionalDelaySamps = (int)(additionalRefDelaySec * sendSampleRateHz + 0.5f);
        pMMFxConfig->RefProcConfig.numDelaySamples = additionalDelaySamps;
    }

    // initialize aec monitor
    {
        AecMonitorConfig_t* pAecMonitorCfg = MMFxGetAecMonitorConfig(pMMFxConfig    );   
 
        // ryu 2013.03
        // MinGainSpeakerphone controls the reported ERL (in power)
        // for demo6mic, we never really know what the system ERL is going to be 
        // for a given demo, so it's a good idea to review the min/max settings
        //            0.5    -> -3 dB
        //            0.4    -> -4 dB
        //            0.25   -> -6 dB
        //            0.2    -> -7 dB
        //            0.1    -> -10 dB
        //            0.05   -> -13 dB
        //            0.025  -> -16 dB
        //            0.03   -> -15 dB
        //            0.015  -> -18 dB
        //            0.0032 -> -25 dB
        //            0.001  -> -30 dB
        // !!! hjm -- these seem a little light
        pAecMonitorCfg->ErlMaxGainSpeakerphone = 0.25f;   // for very low level microphone gain to avoid clipping due to echo
        pAecMonitorCfg->ErlMinGainSpeakerphone = 0.2f;    // such as loud Bluetooth speaker.
        pAecMonitorCfg->ErlInitialGain = 0.25f;

        // probably a don't care, but there is no AEC, maybe set to 1.0, but that might
        // cause some weirdness !!! hjm
        pAecMonitorCfg->ErleMinGainOp = 0.0032f;  // -25 dB
    }
    
    MMIfSetupScratchMemory(pMMFxConfig,
                           pScratchH, MMIF_LEN_SCRATCH_H, 
                           pScratchX, MMIF_LEN_SCRATCH_X,
                           pScratchS, MMIF_LEN_SCRATCH_S);
    ConfigAec(pMMFxConfig, sendSampleRateHz, AEC_LEN_CHAN_MODEL_SEC);

    // update non default parameters for the Frequency Domain beam search
    pMMFxConfig->FdBeamSearchConfig.NumMics = (uint16)MMIfNumMics;
    pMMFxConfig->FdBeamSearchConfig.NumBeamsToSearch = (uint16)MMIfNumSearchBeams;

    for( ib=0; ib<pMMFxConfig->FdBeamSearchConfig.NumBeamsToSearch; ib++ )
        pMMFxConfig->FdBeamSearchConfig.FdBeamSpecObjPtr[ib] = &FdBeamArray[ib];

    pMMFxConfig->MicrophoneLocations_mm = &microphoneLocations_mm[0];
    pMMFxConfig->FdBeamSearchConfig.SubbandWeightsPtrQ10 = &SubbandWeightsQ10[0];
    pMMFxConfig->FdBeamSearchConfig.BeamWeightsPtr = &BeamWeights[0];
    pMMFxConfig->FdBeamSearchConfig.EchoConfidenceThreshQ15 = 4096;  // 0.125
    pMMFxConfig->FdBeamSearchConfig.EnableDownsample2to1 = 0;   // == 1 means do 2:1 downsampling for fdsearch
    pMMFxConfig->NominalDistanceCmFlt = 100.0f;   // 100 cm

    pMMFxConfig->FdBeamSearchConfig.TimeResolution_usec = 10000;
    if (pMMFxConfig->FdBeamSearchConfig.TimeResolution_usec == 10000)
    {
        pMMFxConfig->FdBeamSearchConfig.AlphaConfidenceUp = 0.5f;   // about a 30 ms average
        pMMFxConfig->FdBeamSearchConfig.AlphaConfidenceDown = 0.01f;    // about one second
        pMMFxConfig->FdBeamSearchConfig.AlphaMaxBeamMerit = 0.7f;     // about 8-10 ms time constant
        pMMFxConfig->FdBeamSearchConfig.AlphaReverb = 0.7f;     // about 8-10 ms time constant
        pMMFxConfig->FdBeamSearchConfig.ReverbFactor = 0.75f;     // about 8-10 ms time constant
        pMMFxConfig->FdBeamSearchConfig.BeamMeritDeductFactor = 6.0f;
        pMMFxConfig->FdBeamSearchConfig.MeritRatioExponent = 1.0f;
        pMMFxConfig->FdBeamSearchConfig.EdgeRatioExponent = 1.5f;
        pMMFxConfig->FdBeamSearchConfig.PowerCompareValue = 20.0f;    // was 40, but lowered normalizer gain
    }
    else if (pMMFxConfig->FdBeamSearchConfig.TimeResolution_usec == 2500)
    {
        pMMFxConfig->FdBeamSearchConfig.AlphaConfidenceUp = 0.5f;      // about a 30 ms average
        pMMFxConfig->FdBeamSearchConfig.AlphaConfidenceDown = 0.0025f;   // about one second
        pMMFxConfig->FdBeamSearchConfig.AlphaMaxBeamMerit = 0.25f;     // about 8-10 ms time constant
        pMMFxConfig->FdBeamSearchConfig.AlphaReverb = 0.3f;     // about 8-10 ms time constant
        pMMFxConfig->FdBeamSearchConfig.ReverbFactor = 0.75f;     // about 8-10 ms time constant
        pMMFxConfig->FdBeamSearchConfig.BeamMeritDeductFactor = 6.0f;
        pMMFxConfig->FdBeamSearchConfig.MeritRatioExponent = 2.0f;
        pMMFxConfig->FdBeamSearchConfig.EdgeRatioExponent = 1.5f;
        pMMFxConfig->FdBeamSearchConfig.PowerCompareValue = 20.0f;
    }
    else if (pMMFxConfig->FdBeamSearchConfig.TimeResolution_usec == 1250)
    {
        pMMFxConfig->FdBeamSearchConfig.AlphaConfidenceUp = 0.5f;      // about a 30 ms average
        pMMFxConfig->FdBeamSearchConfig.AlphaConfidenceDown = 0.00125f;   // about one second
        pMMFxConfig->FdBeamSearchConfig.AlphaMaxBeamMerit = 0.125f;     // about 8-10 ms time constant
        pMMFxConfig->FdBeamSearchConfig.AlphaReverb = 0.2f;     // about 8-10 ms time constant
        pMMFxConfig->FdBeamSearchConfig.ReverbFactor = 0.75f;
        pMMFxConfig->FdBeamSearchConfig.BeamMeritDeductFactor = 6.0f;
        pMMFxConfig->FdBeamSearchConfig.MeritRatioExponent = 1.0f;
        pMMFxConfig->FdBeamSearchConfig.EdgeRatioExponent = 1.5f;
        pMMFxConfig->FdBeamSearchConfig.PowerCompareValue = 20.0f;
    }
    else
        SeAssert(0);  // nothing valid, quit

    // Patch in 3 subbands instead of 4 in spatial filter spec
    SpatialFilterSpec.NumSubbands = 3;

    // update non-default parameters for spatial filter
    SpatialFilterInterpretSpecIntoConfig( &SpatialFilterSpec,                    // input
                                          &LocationToBeamMapping,                // input
                                          &pMMFxConfig->SpatialFilterConfig );   // result

    // signal level normalizer: correct mic gains after AEC
    pMMFxConfig->SignalLevelNormConfig.gainSinDb = 0.0f;

    // Set up Spatial filter coefficients
    MMIfSetupSpatialFilterCompensatorCoefficients(&pMMFxConfig->SpatialFilterConfig, &CompensatorMap[0][0], CompensatorList, MAX_SOLUTIONS, MAX_SUBBANDS_EVER);

    pMMFxConfig->SignalLevelNormConfig.gainSinDb = 18.0f; // 24.0f;
    pMMFxConfig->GSout_q10 = 1400;  // +3 dB
    {
        MMPreProcConfig_t *pConfig = MMFxGetPreProcessorConfig(pMMFxConfig);

        // pConfig->CutoffHz = 200.0f; // preprocessor DC removal cutoff, in Hz, default is already 200 Hz
        // reorder to correspond to designed search beams and original clockwise ordering
        //     as designed            old actual                new actual
        //     1       2              2       3             0       1
        //             front                  front                 front
        //     0       3              0       1             2       3
        //
        // where pChannelRemap value represents the value of the target slot
        // and i of pChannelRemap[i] represents the slot of the data as it comes in
        // was with old actual

//#define PROTO_ARRAY 1
#ifdef PROTO_ARRAY
        // mapping with proto board
        pConfig->pChannelRemapSinSourceIndex[0] = 0;
        pConfig->pChannelRemapSinSourceIndex[3] = 1;
        pConfig->pChannelRemapSinSourceIndex[1] = 2;
        pConfig->pChannelRemapSinSourceIndex[2] = 3;
#else
        // new actual remapping compatible with Anki's actual robot
        //                                [mic] = sin ch
        pConfig->pChannelRemapSinSourceIndex[0] = 2;
        pConfig->pChannelRemapSinSourceIndex[1] = 0;
        pConfig->pChannelRemapSinSourceIndex[2] = 1;
        pConfig->pChannelRemapSinSourceIndex[3] = 3;
#endif
    }
    
    // INIT THE MMFX PROCESSOR
    MMFxInit( pMMFxPubObj, pMMFxConfig );

    // after MMFx has been initialized, initialize proj state
    // and publish mmif-level sediags.
    // we do this AFTER MMFxInit because sediag is then available
    InitMMIfState(&MMIfState);
    PublishDiags(&MMIfState);

    // initialize receive path
    RcvIfInit(blockTimeS, rxSampleRateHz);

    isMMIfInitialized = 1;

    // register policy actions init and callback
    // set up policy algorithm AFTER MMFx initialized
    PolicyInit();
    MMIfSetPolicyActions(PolicyDoActions,
                         NULL); // optional argument pointer
}

//
// free all resources
void MMIfDestroy(void)
{
    SeRcvDestroy(SeRcvGetSingletonPubObject());
    MMFxDestroy(MMFxGetSingletonPubObject());
    isMMIfInitialized = 0;
}

/*=======================================================================
  Process microphone and reference signal through tx path

  =======================================================================*/
void MMIfProcessMicrophones(
    const int16 *refPtr,
    const int16 *sinPtr,
    int16 *soutPtr )
{            
    MMFxPublicObject_t* pMMFxPubObj;

    pMMFxPubObj = MMFxGetSingletonPubObject();
    SeAssert(NULL != pMMFxPubObj);

    // operating mode has changed?
    UpdateOpMode(&MMIfState);

    // RUN MMFX ALGORITHM
    MMFxProcessMicrophones( pMMFxPubObj, 
                            sinPtr, 
                            refPtr,
                            soutPtr);
}

/*=======================================================================
  Function Name:   RcvIfProcessReceivePath()

  Notes:
  Preprocess receive-side signal (rin) prior to the loudspeaker (rout)
  =======================================================================*/
void RcvIfProcessReceivePath(
    const int16 *rinPtr, 
    int16 *routPtr 
    )
{
    MMFxPublicObject_t* pMMFxPubObj;
    SeRcvPublicObject_t* pSeRcvPubObj;

    pMMFxPubObj = MMFxGetSingletonPubObject();
    pSeRcvPubObj = SeRcvGetSingletonPubObject();

    SeAssert(NULL!=pMMFxPubObj);
    SeAssert(NULL!=pSeRcvPubObj);

    SeProcessRcv( pSeRcvPubObj,  
                  &pMMFxPubObj->SeRcvParams,
                  rinPtr,
                  routPtr );             
}

//
// set the operating mode (=noise reduction mode)
void MMIfSetOperatingMode(MMIfOpMode_t mode)
{
    MMIfState_t *pState = &MMIfState;

    // set the variable; actual changes occur in UpdateOpMode
    pState->requestedOperatingMode = mode;
}

//
// get the current operating mode (=noise reduction mode)
MMIfOpMode_t MMIfGetOperatingMode(void)
{
    MMIfState_t *pState = &MMIfState;
    return pState->currOperatingMode;
}

// get the maximum value weights for perbin and fullband Noise Reference weights
void GetNoiseRefWeights(float *perBinWeightPtr, float *fullbandWeightPtr)
{
    float perBinWeight;
    float fullbandWeight;

    // follow convention of using pointer rather than static address
    MMIfState_t *pState;
    pState = &MMIfState;

    switch (pState->currOperatingMode)
    {
    case MMIF_TELECOM_MODE:
        perBinWeight = pState->senrConfigTelecom.NrWeightFactor;
        fullbandWeight = pState->senrConfigTelecom.NrFullbandWeightFactor;
        break;

    case MMIF_SPEECH_RECO_MODE:
        perBinWeight = pState->senrConfigSpeechReco.NrWeightFactor;
        fullbandWeight = pState->senrConfigSpeechReco.NrFullbandWeightFactor;
        break;

    default:
        perBinWeight = pState->senrConfigSpeechReco.NrWeightFactor;
        fullbandWeight = pState->senrConfigSpeechReco.NrFullbandWeightFactor;
        break;
    }

    // return the 2 weights
    *perBinWeightPtr = perBinWeight;
    *fullbandWeightPtr = fullbandWeight;
}
