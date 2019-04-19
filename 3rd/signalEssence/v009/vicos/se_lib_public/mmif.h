#ifdef __cplusplus
extern "C" {
#endif
    
/*                                                                           
***************************************************************************
(C) Copyright 2008 SignalEssence; All Rights Reserved

  Module Name  - mmif.h

  Author: Hugh McLaughlin

  Description:

      Contains the Multi-Microphone Audio Algorithm Interface 
      layer definitions.

  History:    hjm - Hugh McLaughlin

  Mar11,09    hjm  first created
  Sep07,11    hjm  modified for Smart Tech
  Feb21,12    ryu  adapted to "core MMIF API"

  Machine/Compiler: ANSI C, Visual C
                    
**************************************************************************
*/

#ifndef ___MMIF_CORE
#define ___MMIF_CORE
  
#include "se_types.h"
#include "mmfxpub.h"
/*=======================================================================

Initialize MMIF layer and se_lib

=======================================================================*/
void MMIfInit(float32 additionalRefDelaySec, void *pArgs);

/*=======================================================================

  Free all resources

=======================================================================*/
void MMIfDestroy(void);

/*=======================================================================
  Process microphone and reference signal through TX path

=======================================================================*/
void MMIfProcessMicrophones(const int16 *refPtr, const int16 *sinPtr, int16 *soutPtr );


/*=======================================================================
  Process RX path input signal
=======================================================================*/
void RcvIfProcessReceivePath(const int16 *rinPtr, 
                             int16 *routPtr);

/*=======================================================================
  HELPER FUNCTIONS

  The remaining API functions are intended to support a user's most common
  cases, e.g. controlling location search, setting the gain.

  For module-specific diagnostics and control, use the SEDiag facility.  
=======================================================================*/

//
// basic configuration parameters
int32 MMIfGetNumMicrophones(void);
float32 MMIfGetSampleRateHz(MMIfPortID_t port);
int32 MMIfGetBlockSize(MMIfPortID_t port);
int32 MMIfGetNumChans(MMIfPortID_t port);
    
    
DEPRECATED(uint16 MMIfGetInputBlockSize(void));

//
// control location search
void MMIfSetAutoLocationSearch( void );
uint16  MMIfGetAutoLocationBypassState( void );
void MMIfGetCurrentLocation( uint16 *indexPtr, int16 *confidencePtr );
void MMIfSetManualLocation( uint16 locationIndex, int16 confidence );

// control final spatial filter selection
void MMIfSetForcedSpatialBeamAllSubbands( uint16 beamIndex );   // forces
void MMIfClearForcedSpatialBeam( void );                        // puts back to normal
uint16 MMIfGetCurrentSpatialBeam(void);
void MMIfSetForcedNrSpatialBeamAllSubbands( uint16 beamIndex );   // forces
void MMIfClearForcedNrSpatialBeam( void );                        // puts back to normal


//
// control receive path (speaker) gain
int16 MMIfGetRoutGainQ10(void);
void MMIfSetRoutGainQ10(int16 gain_q10);
void MMIfBypassRxFullDuplexHelp(int32 onOff);
#define MMIfGetSendPathGainQ10 MMIfGetSoutGainQ10  // for backwards compat
#define MMIfSetSendPathGainQ10 MMIfSetSoutGainQ10 // for backwards compat


//
// control send path output gain (gsout)
int16 MMIfGetSoutGainQ10(void);
void  MMIfSetSoutGainQ10(int16 gain);
#define MMIfGetReceivePathGainQ10 MMIfGetRoutGainQ10 // for backwards compat
#define MMIfSetReceivePathGainQ10 MMIfSetRoutGainQ10 // for backwards compat

//
// AEC controls
uint16 MMIfGetAecBypass(void);
void   MMIfSetAecBypass(uint16 onOff);
const int16* MMIfGetAecChanModelCoef(int32 micIndex);
uint16 MMIfGetAecLenChanModel(void);
void MMIfSetAecChanModelUpdateMode(AdaptiveFilterModes_t mode);

//
// control noise reduction bypass
void MMIfSetNoiseReductionBypass(uint16 onOff); // 0-normal, 1-bypass
uint16 MMIfGetNoiseReductionBypass( void );
void MMIfSetNoiseReductionSelectorTo_OutputOptSoln( void );
void MMIfSetNoiseReductionSelectorTo_OutputNoiseReference( void );

// 
// get version
const char* MMIfGetVersionString(void);


//
// narrowband noise generator test
//======================================================================
// initiate narrowband noise test
// when testing completes, results will automatically appear in MMIfNoiseTestResults[] 
void MMIfStartNoiseTest(void);
//
// returns 1 if narrowband noise test is running
int MMIfNoiseTestIsRunning(void);

//
// return results of most recently-run narrowband noise test
// the results are stored in a float array, dimensions [MAX_MICS][NBNG_MAX_BANDS], with values in dB
float32 *MMIfGetNoiseTestResults(void);

//
// mic calibration
//======================================================================
void MMIfCalibrateMics(void);
int MMIfCalibrationIsRunning(void);
int  MMIfGetCalibrationMicGains(int numMics, float32 *pGainPerMic_flt);

// inject test signal  into rout and sout
//======================================================================
void MMIfEnableWhiteNoiseInjection(void);
void MMIfEnableSineInjection(void);

//
// return to normal processing mode
//======================================================================
void MMIfEnableNormalMode(void);

//
// mic passthru
// route microphone N to sout
// specify valid microphone index or -1 for refIn
//======================================================================
void MMIfEnableMicPassThru(int16 micIndex);

//
// set/get per-mic gain
void MMIfSetTxPreprocGainPerMic(float32 *pGain);
void MMIfGetTxPreprocGainPerMic(float32 *pGain);

void MMIfSetSpatialFilterBypass(uint16 onOff, uint16 bypassMic);
void MMIfSetDramaticChangeFlag(uint16 onOff);

void MMIfSetSpatialFilterBaseEnableFlags( int16 *i16Ptr );

//
// given the version ID string,
// publish a sediag called "mmif_version_id" which
// returns the version ID
void MMIfPublishIdString(char *pId);

void MMIfReinitAecMonitor(MMFxPublicObject_t *pMMFxPub,
                          const AecMonitorConfig_t *pConfig);

/*
policy algorithms
-----------------
declare policy callback functions like this:
void PolicyCallback(void *p)

The policy callback is invoked after the location search

Then use MMIfSetPolicyActions() to register the callback function.
*pArg is an optional argument pointer that will be passed to the callback
functions; set *pArg to NULL if you don't need it
*/
void MMIfSetPolicyActions(MMIfPolicyFunc_t callbackFunc,
                          void *pArg);


void MMIfSetupSpatialFilterCompensatorCoefficients(
    SpatialFilterConfig_t *sfcp,
    int    *compensator_map,
    int16 **compensator_list,
    int     max_solutions,
    int     max_subbands_ever
    );

#endif  // __MMIF_CORE

#ifdef __cplusplus
}
#endif // linkage 
