/**
* File: sensoryStaticData.c
*
* Author: Lee Crippen
* Created: 12/12/2016
*
* Description: Pulls in static version of english data model as a prototype, in place of loading from file.
*
* Copyright: Anki, Inc. 2016
*
*/

#if VOICE_RECOG_PROVIDER == VOICE_RECOG_THF

#include "sensoryStaticData.h"

#define STATIC_NETFILE    "../../../../../../EXTERNALS/coretech_external/sensory/TrulyHandsfreeSDK/4.4.4/Data/en_us_16kHz_v10/nn_en_us_mfcc_16k_15_250_v5.1.1.c" /* Acoustic model static */
#define STATIC_LTSFILE    "../../../../../../EXTERNALS/coretech_external/sensory/TrulyHandsfreeSDK/4.4.4/Data/en_us_16kHz_v10/lts_en_us_9.5.2b.c" /* Pronunciation model static*/

// Stub long location path to make updating relative paths above a little easier.
// cozmo-one/basestation/src/anki/cozmo/basestation/voiceCommands/../../../../../../EXTERNALS/coretech_external/sensory/TrulyHandsfreeSDK/4.4.4/Data/en_us_16kHz_v10

#define INDEX 0
#include STATIC_NETFILE
#undef INDEX

static const void *recogs[] = {&recog0, };
sdata_t thf_static_recog_data = {1, (const void **)recogs};

#define INDEX 0
#include STATIC_LTSFILE
#undef INDEX
static const void *pronuns[] = {&pronun0, &phonrules0,
#ifdef HAVE_textnorm
  &textnorm0,
#undef HAVE_textnorm
#else
  NULL,
#endif
#ifdef HAVE_ortnormA
  &ortmormA0,
#undef HAVE_ortnormA
#else
  NULL,
#endif
#ifdef HAVE_ortnormB
  &ortnormB0,
#undef HAVE_ortnormB
#else
  NULL,
#endif
};

sdata_t thf_static_pronun_data = {5, (const void **)pronuns};


#endif // VOICE_RECOG_PROVIDER == VOICE_RECOG_THF
