#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2009 Signal Essence; All Rights Reserved

 Module Name  - mmfxpub.h

 Author: Hugh McLaughlin

 History
     created Mar13,09

 Description:
*/

#ifndef __mmfxpub_h
#define __mmfxpub_h

#include "aecmonitorpub.h"
#include "buffer_composer_pub.h"
#include "fdsearchconfig.h"
#include "highpass_filter_array_pub.h"
#include "keyword_recog_pub.h"
#include "lec_pub.h"
#include "mmfxcalibactionspub.h"
#include "mmglobalsizes.h"
#include "mmpreprocessor_pub.h"
#include "multiaecpub.h"
#include "output_injector_pub.h"
#include "policy_actions_types.h"
#include "ref_proc_pub.h"
#include "scratch_mem_pub.h"
#include "se_nr_pub.h"
#include "se_types.h"
#include "sercvpub.h"
#include "signal_level_norm_pub.h"
#include "spatialfilterconfig.h"
#include "system_tests_pub.h"
#include "tap_point.h"

#define SPEED_OF_SOUND_20C_METERS_PER_SEC 343.26f
#define SPEED_OF_SOUND_20C_MM_PER_MS      SPEED_OF_SOUND_20C_METERS_PER_SEC
#define SPEED_OF_SOUND_20C_CM_PER_MS      (0.1f * SPEED_OF_SOUND_20C_MM_PER_MS)


//
// pointers to private object AND control fields
typedef struct
{
    void    *VoidPrivateMMFxObjPtr;  /* pointer to private data structure */

    int16   GSout_q10;             // Sout gain, final stage, q10 number, 1024 = Unity
    int16   SoutRmsLimit;          // RMS Limit on the final stage compressor-limiter 

    // Bypass and Enable Flags
    uint16  BypassDcRmv;
    uint16  BypassEchoCanceller;
    uint16  BypassLocationSearch;
    uint16  BypassSpatialFilter;
    uint16  BypassSpatialFilterMicIndex;  // if spatial filter bypassed, select this mic index
    uint16  BypassSoutRmsLimiter;
    uint16  EnableLineEchoCanceller;      // by default, LEC is disabled
    SenrOutput_t  SenrOutputSelector;
    uint16  LocationBasedAgcEnabled;
    uint16  UseHeadset;

    uint16 ForceLocationIndex;       // set in tandem with ForceLocationFlag
    int16  ForceLocationConfidence;  // set in tandem with ForceLocationFlag

    // messages from send path to receive path
    SeRcvParamsFromTxPath_t SeRcvParams;

    // tappoints for inspecting/injecting signal
    //
    // tappoints are defined in the public structure
    // because the customer may need access for customization
    TapPoint_t TapPreFdSearch;
    TapPoint_t TapPostFdSearch;
    TapPoint_t TapPreSpatialFilter;
    TapPoint_t TapPostSenr;
    TapPoint_t TapPostFinalGain;

    //
    // policy algorithm
    // callback function and argument
    MMIfPolicyFunc_t      PolicyCallback;
    void                  *pPolicyArg;

    //
    // post-spatial filter highpass filter
    int16 PostSfHpfNumPoles;
    float32 PostSfHpfCutoffHz;

} MMFxPublicObject_t;


//
// MMFxConfig specifies one-time configuration parameters.
// The user should treat this structure as write-only.
typedef struct
{
    //
    // configuration parameters for MMFx components
    SEDiagConfig_t SEDiagConfig;
    AecMonitorConfig_t AecMonitorConfig;
    MultiAecConfig_t MultiAecConfig;
    FdBeamSearchConfig_t    FdBeamSearchConfig;
    MMFxCalibrationConfig_t CalibrationConfig;
    SpatialFilterConfig_t   SpatialFilterConfig;
    ScratchMemConfig_t      ScratchMemConfig;
    MMPreProcConfig_t MMPreProcConfig;
    SenrConfig_t SenrConfig;
    SystemTestsConfig_t SystemTestsConfig;     // narrowband noise generator sweep parameters, sine generator
    LineEchoCancellerConfig_t LecConfig;

    //
    // move this into tx gain stage config object
    // final tx gain stage
    int16  GSout_q10;          // Sout gain, final stage, q10 number, 1024 = Unity
    int16  SoutRmsLimit;       // compressor-limiter RMS limit, typically 8000-ish 

   
    // Spatial Filter-Specific Configuration
    // !!! ryu: move these fields to spatial filter config object
    float  NominalDistanceCmFlt;
    MicrophoneLocation_mm *MicrophoneLocations_mm;      // Locations of microphones

    RefProcConfig_t RefProcConfig;

    InjectorConfig_t InjectorConfig;

    //
    // post-spatial filter highpass filter
    HighpassFilterArrayConfig_t PostSfHpfConfig;

    int32 NumSfOutputChannels;

    //
    // These parameters are set by the the mmfx set defaults
    // and should not be touched after the call to setdefaults.
    uint16     NumMics;
    uint16     NumSinChannels;
    float32    BlockTimeS; 
    float32    SampleRateHz_Send;   // the working sample rate
    float32    SampleRateHz_Sin;
    float32    SampleRateHz_Sout;
    float32    SampleRateHz_Refin;

    int32 BlockSize_Send; // working blocksize
    int32 BlockSize_Sin;
    int32 BlockSize_Sout;
    int32 BlockSize_Refin;

    SlnConfig_t SignalLevelNormConfig;

    BufferCompConfig_t SoutComposerConfig;
} MMFxConfig_t;

/* Function Prototypes */
void MMFxSetDefaultConfig(MMFxConfig_t *cfgp,
                          int32 numMics,
                          float32 blockTimeS,
                          float32 sampleRateHz_Send,
                          float32 sampleRateHz_Sin,
                          float32 sampleRateHz_Sout,
                          float32 sampleRateHz_Refin,
                          float32 sampleRateHz_Rcv,
                          float32 sampleRateHz_Rin,
                          float32 sampleRateHz_Rout
    );

/*=======================================================================
   Function Name:   MMFxInit()
 
   Description:     Initializes everything.  Presumes that the proper
                    memory has been allocated.
=======================================================================*/
int16 MMFxInit( MMFxPublicObject_t *pubp,
                MMFxConfig_t *cfgp );

//
// free/reset resources allocated during MMFxInit
void MMFxDestroy(MMFxPublicObject_t *pubp);


void MMFxProcessMicrophones( 
         MMFxPublicObject_t *pp, 
         const int16 *sinPtr, 
         const int16 *refPtr,
         int16 *outPtr );

/*=======================================================================

  Extract individual config objects from MMFxConfig

=======================================================================*/
MMFxPublicObject_t* MMFxGetSingletonPubObject(void);

AecMonitorConfig_t* MMFxGetAecMonitorConfig(MMFxConfig_t *pConfig);

MultiAecConfig_t* MMFxGetMultiAecConfig(MMFxConfig_t *pConfig);

MMFxCalibrationConfig_t* MMFxGetCalibrationConfig(MMFxConfig_t *pConfig);

SenrConfig_t* MMFxGetNoiseReductionConfig(MMFxConfig_t *pConfig);

ScratchMemConfig_t* MMFxGetScratchMemConfig(MMFxConfig_t *pConfig);

MMPreProcConfig_t* MMFxGetPreProcessorConfig(MMFxConfig_t *pConfig);

BufferCompConfig_t *MMFxGetSoutComposerConfig(MMFxConfig_t *pConfig);

SpatialFilterSpec_t *MMFxGetSpatialFilterConfig(MMFxPublicObject_t *pConfig);
/*=======================================================================

  General setters and getters

=======================================================================*/
int MMFxGetNumMics(MMFxPublicObject_t* pMMFxPubObj);


/* ===========================================================================
    Takes the Spatial Filter Specification and interprets it into
    the Config data structure.   For the most part, this is a one-to-one copy, but
    this could allow for additional interpretation, say from a floating point
    value to fixed point, if ever needed
    
    IN
    *scp - SpatialFilterSpec_t structure

    OUT
    *cp - SpatialFilterConfig_t structure

    RETURNS: void

============================================================================== */   
void SpatialFilterInterpretSpecIntoConfig( SpatialFilterSpec_t     *scp, 
                                           LocationToBeamMapping_t *lbmp,
                                           SpatialFilterConfig_t   *cp );


/* ==================================================================================
    Populates the Spatial Filter Config data structure with reasonable defaults.
    
    IN/OUT:
    *cp - LocationToBeamMapping_t structure

    RETURNS: void
================================================================================== */
void SpatialFilterSetDefaultConfig( SpatialFilterConfig_t *cp, int16 blockSize, int16 sampleRate_kHz );


void CopyLocationToBeamMapping( LocationToBeamMapping_t *srcp,
                                LocationToBeamMapping_t *destp );

void CopySpatialFilterSpec( SpatialFilterSpec_t *srcp, SpatialFilterSpec_t *destp );


void MMFxDisableBeamSearchLocation    ( MMFxPublicObject_t* pMMFxPubObj, uint16 index );
void MMFxEnableBeamSearchLocation     ( MMFxPublicObject_t* pMMFxPubObj, uint16 index );
void MMFxDisableBeamSearchLocationList( MMFxPublicObject_t* pMMFxPubObj, const uint16 *disableList, int N);
void MMFxEnableBeamSearchLocationList ( MMFxPublicObject_t* pMMFxPubObj, const uint16 *enableList , int N);
void MMFxEnableAllBeamSearchLocations ( MMFxPublicObject_t* pMMFxPubObj );

void MMFxSetForcedSpatialBeamAllSubbands( MMFxPublicObject_t *pMMFxPubObj, uint16 beamIndex );
void MMFxClearForcedSpatialBeam( MMFxPublicObject_t *pMMFxPubObj );
void MMIfSetAutoLocationSearch( void );
void MMFxSetForcedNrSpatialBeamAllSubbands( MMFxPublicObject_t *pMMFxPubObj, uint16 beamIndex );
void MMFxClearForcedNrSpatialBeam( MMFxPublicObject_t *pMMFxPubObj );

void MMIfSetupScratchMemory(MMFxConfig_t *pMMFxConfig, 
                            char *pScratchH, size_t lenH,
                            char *pScratchX, size_t lenX,
                            char *pScratchS, size_t lenS);

void MMFxReconfigNoiseReduction(MMFxPublicObject_t *pMMFxPubObj, const SenrConfig_t *pConfig);

uint16 MMFxGetHeadsetMode(void);
void MMFxSetHeadsetMode(uint16 val);


#endif

#ifdef __cplusplus
}
#endif

