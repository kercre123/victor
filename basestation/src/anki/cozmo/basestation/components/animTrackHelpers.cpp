/**
 * File: animTrackHelpers.cpp
 *
 * Author: Lee Crippen
 * Created: 1/13/2016
 *
 * Description: Helpful functionality for dealing with animation tracks.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/components/animTrackHelpers.h"
#include "clad/types/animationKeyFrames.h"
#include "util/math/numericCast.h"

#include <sstream>

namespace Anki {
namespace Cozmo {
  
std::string AnimTrackHelpers::AnimTrackFlagsToString(uint8_t tracks)
{
  std::stringstream ss;
  for (int i=0; i < (int)AnimConstants::NUM_TRACKS; i++)
  {
    uint8_t currTrack = Util::numeric_cast<uint8_t>(1 << i);
    if( tracks & currTrack ) {
      ss << EnumToString( static_cast<AnimTrackFlag>(currTrack) ) << ' ';
    }
  }
  
  return ss.str();
}

  
} // namespace Cozmo
} // namespace Anki
