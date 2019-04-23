#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2010 Signal Essence; All Rights Reserved
* 
*   Module Name  - mmglobalcodes.h
* 
*   Author: Hugh McLaughlin
* 
*   History
*       created Nov08,10 hjm  split from mmglobalparams.h
*       
*   Description:
* 
* **************************************************************************
*/

#ifndef __mmglobalcodes_h
#define __mmglobalcodes_h

// #define FIRST_MIC_OMNI     0
// #define CARDIOID           1
// #define NEAR_BIDIRECTIONAL 2
// #define NEAR_NULL_S3_S5    3

// Pick ONE or None of the following 3 to define
// Clean this shit up someday hjm !!!
//#define IMITATE_2817 1
//#define IMITATE_292MODC 1
// for old 16-bin operation leave undefined, only applies to 8 kHz sample rate
//#define ENABLE_32_GROUPED_BINS_AT_8KHZ  1
//#define ZETA_EMPHASIS_0  1
//#define ZETA_EMPHASIS_0p4 1
#define ZETA_EMPHASIS_0p8 1

#include "se_diag.h"

// Constant numbers
// #define C0_500_Q12 2048
// #define C0_250_Q12 1024
// #define C0_167_Q12 683

//
// narrowband noise generator parameters
#define NBNG_MAX_BANDS    25     // MUST BE SAME AS NB_FILT_COUNT
#define NBNG_MAX_SECTIONS 4

//
// number of points used by fuzzy mapper
// defined here to make it public
#define FUZZY_NUM_CURVE_PNTS 5

// Controls for AEC adaptation
typedef enum {
    SE_AF_FIRST_DONT_USE,
    SE_AF_NORMAL_ADAPTATION,
    SE_AF_DISABLE_ADAPTATION,
    SE_AF_FORCE_ADAPTATION,
    SE_AF_LAST
} AdaptiveFilterModes_t;

//
// the mmfx operating mode specifies, at a high level,
// what processing is performed by the mmfx transmit and receive paths
typedef enum
{
    MMFX_MODE_FIRST_DONT_USE,
    MMFX_MIC_CALIBRATION,          // run mic calibration
    MMFX_MIC_CALIBRATION_ANALYSIS, // do mic calibration analysis
    MMFX_NBNG_TEST,        // narrowband noise test
    MMFX_NORMAL_MODE,  // normal processing
    MMFX_PASSTHRU,     // route mic N directly to sout; rin goes directly to rout
    MMFX_MODE_LAST
} MMFxOpMode_t;


#endif
#ifdef __cplusplus
}
#endif

