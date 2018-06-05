/**
 * File: illuminationState.h
 *
 * Author: Humphrey Hu
 * Date:   2018/05/25
 *
 * Description: Enum for describing state of illumination in the environment.
 * 
 * Copyright: Anki, Inc. 2018
 **/

#include <string>

#ifndef __Anki_Victor_IlluminationState_H__
#define __Anki_Victor_IlluminationState_H__

namespace Anki {
namespace Vision {

enum class IlluminationState {
  Unknown,      // Cannot determine illumination
  Illuminated,  // Environment is well-lit
  Darkened      // Environment is dark, not lit
};

} // end namespace Vision
} // end namespace Anki

#endif // __Anki_Victor_IlluminationState_H__