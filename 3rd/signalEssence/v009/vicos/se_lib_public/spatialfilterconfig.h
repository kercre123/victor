#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2011 Signal Essence; All Rights Reserved
  
  Module Name  - spatialfilterconfig.h
  
  Author: Hugh McLaughlin
  
  History
  
  2011-01-05  hjm  created based on fdsearchconfig.h and mmfxspatialfilter.h
    
  Description:
  
**************************************************************************
*/

#ifndef ___spatialfilterconfig_h
#define ___spatialfilterconfig_h

#include "mmglobalsizes.h"
#include "se_types.h"

typedef struct
{
    int16  WidthThresholdHighConfidence;
    int16  WidthThresholdMediumConfidence;
    int16  WidthThresholdLowConfidence;
    uint16 FallBackBeamIndex;
    uint16 BeamIndex[MAX_SEARCH_BEAMS][MAX_BEAM_WIDTH_INDEXES];  // shouldn't this use MAX_SEARCH_BEAMS instead of MAX_SOLUTIONS
    uint16 NumInSet[MAX_SEARCH_BEAMS][MAX_BEAM_WIDTH_INDEXES];   // shouldn't this use MAX_SEARCH_BEAMS instead of MAX_SOLUTIONS
} LocationToBeamMapping_t;

typedef struct
{
    // configuration parameters
    int16                SampleRate_kHz;
    uint16               BlockSize;
    uint16               Overlap;
    uint16               NumMics;
    int16                NoCompDelay;
    uint16               NumSubbands;
    uint16               NumNoiseRefCandidates;                   // presume the same number per subband -- not currently used
    uint16               NumSolutionsInclNr[MAX_SUBBANDS_EVER];   // number of all beams desired and NR
    uint16               NumSolnCandidates[MAX_SUBBANDS_EVER];    // number of all beams, not including NR
    uint16               NumMicContribs[MAX_SUBBANDS_EVER][MAX_SOLUTIONS];
    int16                BeamNeedsCompensationFlags[MAX_SUBBANDS_ALLOC][MAX_SOLUTIONS];            
    int16                ResultWeightBase[MAX_SUBBANDS_EVER][MAX_SOLUTIONS];
    int16                MicCombineIndexTable[MAX_SUBBANDS_EVER][MAX_SOLUTIONS][MAX_MIC_CONTRIBS];
    int16                MicCombineDelayTable[MAX_SUBBANDS_EVER][MAX_SOLUTIONS][MAX_MIC_CONTRIBS];
    int16                CombineGain[MAX_SUBBANDS_EVER][MAX_SOLUTIONS][MAX_MIC_CONTRIBS];
} SpatialFilterSpec_t;

typedef struct 
{
    SpatialFilterSpec_t     Spec;
    int16                   *CompCoefPtrs[MAX_SUBBANDS_ALLOC][MAX_SOLUTIONS];   
    // The enable solution array is the base at init time.   This gives and easy way
    // to restrict beams at the start, while another array in private is kept to enable
    // and disable beams depending on the direction-finding results.
    int16                   EnableSolutions[MAX_SUBBANDS_EVER][MAX_SOLUTIONS];
    LocationToBeamMapping_t LocationToBeamMapping;
} SpatialFilterConfig_t;

#endif
#ifdef __cplusplus
}
#endif

