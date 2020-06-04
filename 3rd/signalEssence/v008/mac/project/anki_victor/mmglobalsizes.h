#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2009 Signal Essence; All Rights Reserved
* 
*   Module Name  - mmglobalsizes.h for Project Vocera
* 
*   Author: Hugh McLaughlin
* 
*   History
*       created Nov08,10  split from mmglobalparams.h
*   
*   Description:
* 
* **************************************************************************
*/

#ifndef __mmglobalsizes_h
#define __mmglobalsizes_h

// maximum length of acoustic echo canceller estimated channel model (in samples)
#define AEC_MAX_LEN_CHAN_MODEL 4096

// max number of samples in input blocks
#define MAX_BLOCKSIZE             160   
#define MAX_MICS                   15
#define MAX_SEARCH_BEAMS           70    // search beams
#define MAX_SEARCH_CONTRIBUTORS    15
#define MAX_NUM_FD_SUBBANDS         6
#define MAX_BEAM_WIDTH_INDEXES      3

#define MAX_SOLUTIONS              70    // spatial filter beams
#define MAX_MIC_CONTRIBS           30    // differential beams will usually require fewer microphones

// MAX_SUBBANDS is divided up into two purposes:
//  1. The maximum ever anticipated.  This is used to define buffers for configuration
//     parameters.
//  2. The maximum number of buffers needed.  This is used to define the maximum for
//     large buffers.
// This way it is possible to make the memory allocation small or large as needed
// without having to touch a lot of parameters.
#define MAX_SUBBANDS_EVER  4
#define MAX_SUBBANDS_ALLOC 4

#define AUTOGEN_CONFIG_DEFINES_ONLY
#include "autogen_config.h"
#undef AUTOGEN_CONFIG_DEFINES_ONLY

#endif
#ifdef __cplusplus
}
#endif

