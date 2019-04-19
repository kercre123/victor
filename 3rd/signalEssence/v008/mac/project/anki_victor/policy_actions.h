#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2016 Signal Essence for Robot; All Rights Reserved
  
  Module Name  - policy_actions.h
  
  Author: Hugh McLaughlin
  
  History
  
  04-08-14    hjm  created
  
  Description:
     definitions for the policy_actions
**************************************************************************
*/

#ifndef __policy_actions_h
#define __policy_actions_h

#include "se_types.h"
#include "tap_point.h"
#include "mmif_proj.h"   // includes mm_globals.h

#define MAX_POSITION_READINGS 10
#define DIFF_WEIGHT_ALPHA  0.9f

//typedef struct 
//{
//    float Degrees;
//    float Radians;
//    int   ClockDialIndex;
//    float Confidence;
//} DirectionInfo_t;

typedef struct 
{
    // fixed parameters
    //uint16 NumBeamsToSearch;
    //uint16 BlockSize_ms;  // usually 10 or 20 milliseconds
    uint16 NumMics;
    uint32 MaxCountNoMovement;
    float AlphaDownNoMovement;
    float AlphaDownLessThan1Degree;
    float AlphaDown2Degrees;
    float AlphaDown4Degrees;
    float AlphaDown8Degrees;
    float AlphaDown16Degrees;
    float AlphaDownFast;
    int32 KeyPhraseTimeOutCount;  // set at initialization
    int32 KeyPhraseCountDown;     // variable, decrement each block
    int32 OverrideWithPointingLocationFlag; // use relative Phi to set spatial filter if set
    int32 FallbackFlag;                     // use the fallback beam regardless of anything else
    int32 NumReadings;

    int    SEDiagId_SearchResultBestBeamIndex;
    int    SEDiagId_SearchResultBestBeamConfidence;
    int    SEDiagId_NewPhiUpdateMailboxFlag;
    int    SEDiagId_PhiUpdateMailbox;
    int    SEDiagId_KeyPhraseDetectedMailboxFlag;
    int    SEDiagId_KeyPhraseIndexFromHost;
    int    SEDiagId_NR_PerBinWeight;
    int    SEDiagId_NR_FullbandWeight;
    int    SEDiagId_FdSearchConfidenceState;
    int    SEDiagId_FdSearchBestBeamIndex;
    int    SEDiagId_FdSearchBestBeamConfidence;

    float  DiffWeight[MAX_POSITION_READINGS];

    uint32 BlockCounter;
    uint32 CountNoMovement;
    uint32 CountThresholdNoMovement;
//    int    KeyPhraseFlag;
    int    KeyPhraseIndex;

    // These variables are like a mailbox
    // The external software can set by setting the flag.
    // These are meant to be set synchronously with a call
    // to MMFx, so there is no need to protect the read or write.
    // We just do not know when there will be a new set of values.
    // So the external software needs to set a flag when the values are new.
    // The policy actions will clear the flag when the values are read.
    int32 NewPhiMailboxFlag;  // new Phi coordinates
    float NewPhiMailbox;

    // 2nd mailbox for key phrase detection
    int32 KeyPhraseDetectedMailboxFlag;
    int32 KeyPhraseIndexMailbox;        // set to -1 if SE determined, to valid index if host determined

    int32 RelativePhiMailboxFlag;
    float RelativePhiMailbox;

    float NewPhi;

    uint16 InterpolatedBestBeamIndex;
    short InterpolatedBestBeamConfidence;

    float PreviousPhi[MAX_POSITION_READINGS];

    float WeightedDiffRotate[MAX_POSITION_READINGS];   // polar coordinates of new data
    float WeightedDiffPitch[MAX_POSITION_READINGS];

    float DiffPitchMax;
    float DiffRotateMax;

    float BodyRelativePhi;
    float BodyRelativePhiDegrees;  // for info convenience only

    float KeyPhraseAbsolutePhi;
    float KeyPhraseAbsolutePhiDegrees;
    float RelativeOffsetPhi;

    float AlphaDown;
    float TotalAngleMaxDiff;

    //DirectionInfo_t DirectionInfoStruct;

    int SEDiagId_AlphaConfidenceDown;
    int SEDiagId_AlphaConfidenceUp;

} RobotAudioPolicy_t;


// asynchonously report the angular orientation relative to true north
void PolicySetAbsoluteOrientation(float phiRadians);

void PolicyInit(void);

void PolicyDoActions(void *p);

void PolicySetKeyPhraseDirection(int index);

void PolicySetAimingDirection(float relativePhiRadians);

void PolicyDoAutoSearch(void);

void PolicySetFallback(void);
void PolicyClearFallback(void);
void PolicySetFallbackEchoCancelling(void);

#endif

#ifdef __cplusplus
}
#endif

