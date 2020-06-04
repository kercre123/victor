
/*----------------------------------------------------------------------- 
 (C) Copyright 2018 Signal Essence; All Rights Reserved
  autogen_config.h
  Description: Automatically generated configuration from the spatialconfig_writer.h
  This file is Auto-generated.  Do not edit.
  Generated on 2018-11-02 10:41:19.899544 by hugh
----------------------------------------------------------------------- */
#ifndef AUTOGEN_CONFIG_DEFINES_ONLY
#ifdef __cplusplus
extern "C" {
#endif
#ifndef _AUTOGEN_CONFIG_FDSEARCH_H
#define _AUTOGEN_CONFIG_FDSEARCH_H
#include "fdsearchconfig.h"

// variables defined in fdsearchconfig.c
extern int MMIfNumMics       ; // # of microphones
extern int MMIfNumSearchBeams; // total number of beams to search
extern MicrophoneLocation_mm microphoneLocations_mm[];
extern FdBeamSpecObj_t FdBeamArray[];
extern int16 SubbandWeightsQ10[];
extern int16 BeamWeights[];
extern float DirectivityComp[][MAX_NUM_FD_SUBBANDS];
extern SpatialFilterSpec_t     SpatialFilterSpec;
extern LocationToBeamMapping_t LocationToBeamMapping;

#endif 
#ifdef __cplusplus
}
#endif // end of fdsearch header info
#endif // end AUTOGEN_CONFIG_DEFINES_ONLY

/*----------------------------------------------------------------------- 
 (C) Copyright 2018 Signal Essence; All Rights Reserved
  spatialfilterconfig_autogen.h
  Description: Automatically generated configuration from the spatialconfig_writer.h
  This file is Auto-generated.  Do not edit.
  Generated on 2018-11-02 10:41:20.008254 by hugh
----------------------------------------------------------------------- */
#ifdef AUTOGEN_CONFIG_DEFINES_ONLY
        // needed for mmglobalsize.h.  
        // define AUTOGEN_CONFIG_DEFINES_ONLY  in mmglobalsizes.h to just get these defines.
        // needed for recursive include file
        #define COMPENSATOR_LENGTH (180)
#else
#    ifdef __cplusplus
        extern "C" {
#    endif
#    ifndef _AUTOGEN_CONFIG_SPATFILT_H
#        define _AUTOGEN_CONFIG_SPATFILT_H
#        include "se_types.h"
#        include "spatialfilterconfig.h"
        extern int CompensatorListLength;
extern int16 *CompensatorList[21];
extern int CompensatorLength[21];
extern int CompensatorMap[MAX_SOLUTIONS][MAX_SUBBANDS_EVER];

#    endif // _AUTOGEN_CONFIG_SPATFILT_H
#    ifdef __cplusplus
           }
#    endif // __cplusplus
#endif // AUTOGEN_CONFIG_DEFINES_ONLY
