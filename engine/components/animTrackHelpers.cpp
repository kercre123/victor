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

#include "engine/components/animTrackHelpers.h"
#include "clad/types/animationTypes.h"
#include "util/math/numericCast.h"

#include <sstream>

namespace Anki {
namespace Vector {
  
std::string AnimTrackHelpers::AnimTrackFlagsToString(uint8_t tracks)
{
  // Special case of 0x00?
  if (AnimTrackFlag::NO_TRACKS == static_cast<AnimTrackFlag>(tracks)) {
    return EnumToString(AnimTrackFlag::NO_TRACKS);
  }
  
  // Special case of 0xff?
  if (AnimTrackFlag::ALL_TRACKS == static_cast<AnimTrackFlag>(tracks)) {
    return EnumToString(AnimTrackFlag::ALL_TRACKS);
  }
  
  // General case of one or more tracks
  std::stringstream ss;
  bool first = true;
  
  for (int i=0; i < (int)AnimConstants::NUM_TRACKS; i++)
  {
    uint8_t currTrack = Util::numeric_cast<uint8_t>(1 << i);
    if (tracks & currTrack) {
      if (first) {
        first = false;
      } else {
        ss << '+';
      }
      ss << EnumToString(static_cast<AnimTrackFlag>(currTrack));
    }
  }
  
  return ss.str();
}

  
} // namespace Vector
} // namespace Anki
