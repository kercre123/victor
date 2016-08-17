/*
 * File: audioMixingTypes.h
 *
 * Author: Jordan Rivas
 * Created: 05/17/16
 *
 * Description:
 *
 * #include "anki/cozmo/basestation/audio/mixing/audioMixingTypes.h"
 *
 * Copyright: Anki, Inc. 2016
 */


#ifndef __Basestation_Audio_AudioMixingTypes_H__
#define __Basestation_Audio_AudioMixingTypes_H__

#include "anki/cozmo/basestation/audio/audioDataTypes.h"
#include <functional>

namespace Anki {
namespace Cozmo {
namespace Audio {

class AudioMixingConsole;
  
using MixingConsoleSample = double;
using MixingConsoleBuffer = MixingConsoleSample*;

  
} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_AudioMixingTypes_H__ */
