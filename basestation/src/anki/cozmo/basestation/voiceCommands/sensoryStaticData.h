/**
* File: sensoryStaticData.h
*
* Author: Lee Crippen
* Created: 12/12/2016
*
* Description: Pulls in static version of english data model as a prototype, in place of loading from file.
*
* Copyright: Anki, Inc. 2016
*
*/
#ifndef __Anki_VoiceCommands_SensoryStaticData_H_
#define __Anki_VoiceCommands_SensoryStaticData_H_


#ifdef __cplusplus
extern "C" {
#endif
  
#if VOICE_RECOG_PROVIDER == VOICE_RECOG_THF
  
#include "trulyhandsfree.h"
  
extern sdata_t thf_static_recog_data;
extern sdata_t thf_static_pronun_data;
  
#endif // VOICE_RECOG_PROVIDER == VOICE_RECOG_THF

  
#ifdef __cplusplus
}
#endif

#endif // __Anki_VoiceCommands_SensoryStaticData_H_
