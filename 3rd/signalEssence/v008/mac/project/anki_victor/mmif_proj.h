#ifdef __cplusplus
extern "C" {
#endif

/*                                                                           
***************************************************************************
 (C) Copyright 2016 SignalEssence; All Rights Reserved

**************************************************************************
*/

#ifndef ___semmif_h
#define ___semmif_h

#include "se_types.h"
#include "mmglobalsizes.h"
#include "mmif.h"

//
// set project-specific operating mode
typedef enum {
    MMIF_FIRST_DONT_USE,
    MMIF_SPEECH_RECO_MODE,
    MMIF_TELECOM_MODE,
    MMIF_LAST
} MMIfOpMode_t;

// use with SEDIAG policy_fallback_flag
typedef enum
{
    FBF_FIRST_DONT_USE,     // do not use
    FBF_AUTO_SELECT,        // auto-select beam
    FBF_FORCE_FALLBACK,     // select fallback beam
    FBF_FORCE_ECHO_CANCEL,  // select echo-cancelling beam
    FBF_LAST                // do not use
} FallbackFlag_t;

void MMIfSetOperatingMode(MMIfOpMode_t mode);
MMIfOpMode_t MMIfGetOperatingMode(void);

//
// modify location search parameters to ignore gear noise
void MMIfSetGearNoiseThreshold(float32 thresh);
float32 MMIfGetGearNoiseThreshold(void);

// reset location search confidence values
void MMIfResetLocationSearch(void);

#endif
#ifdef __cplusplus
}
#endif

