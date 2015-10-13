/**
* File: namedColors
*
* Author: damjan stulic
* Created: 9/16/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/


#include "anki/cozmo/basestation/namedColors/namedColors.h"


namespace Anki {
namespace NamedColors {

// Add some BlockWorld-specific named colors:
const ColorRGBA EXECUTED_PATH
  (1.f, 0.0f, 0.0f, 1.0f);
const ColorRGBA PREDOCKPOSE
  (1.f, 0.0f, 0.0f, 0.75f);
const ColorRGBA PRERAMPPOSE
  (0.f, 0.0f, 1.0f, 0.75f);
const ColorRGBA SELECTED_OBJECT
  (0.f, 1.0f, 0.0f, 0.0f);
const ColorRGBA BLOCK_BOUNDING_QUAD
  (0.f, 0.0f, 1.0f, 0.75f);
const ColorRGBA OBSERVED_QUAD
  (1.f, 0.0f, 0.0f, 0.75f);
const ColorRGBA ROBOT_BOUNDING_QUAD
  (0.f, 0.8f, 0.0f, 0.75f);
const ColorRGBA REPLAN_BLOCK_BOUNDING_QUAD
  (1.f, 0.1f, 1.0f, 0.75f);
const ColorRGBA LOCALIZATION_OBJECT
  (1.0, 0.0f, 1.0f, 1.0f);


} // end namespace NamedColors
} // end namespace Anki
