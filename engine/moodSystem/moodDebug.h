/**
 * File: moodDebug
 *
 * Author: Mark Wesley
 * Created: 11/20/15
 *
 * Description: Switches etc. for enabling mood debugging
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_MoodSystem_MoodDebug_H__
#define __Cozmo_Basestation_MoodSystem_MoodDebug_H__


#include "util/global/globalDefinitions.h"


#if ANKI_DEV_CHEATS
  #define SEND_MOOD_TO_VIZ_DEBUG  1
#else
  #define SEND_MOOD_TO_VIZ_DEBUG  0
#endif // ANKI_DEV_CHEATS

#if SEND_MOOD_TO_VIZ_DEBUG
  #define SEND_MOOD_TO_VIZ_DEBUG_ONLY(exp)  exp
#else
  #define SEND_MOOD_TO_VIZ_DEBUG_ONLY(exp)
#endif // SEND_MOOD_TO_VIZ_DEBUG



#endif // __Cozmo_Basestation_MoodSystem_MoodDebug_H__

