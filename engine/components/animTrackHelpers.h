/**
 * File: animTrackHelpers.h
 *
 * Author: Lee Crippen
 * Created: 1/13/2016
 *
 * Description: Helpful funcitonality for dealing with animation tracks.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_AnimTrackHelpers_H__
#define __Anki_Cozmo_Basestation_Components_AnimTrackHelpers_H__

#include <string>

namespace Anki {
namespace Vector {
  
enum class AnimTrackFlag : uint8_t;

class AnimTrackHelpers
{
public:
  // Turns animation track flags into a space separated string for easy debugging
  static std::string AnimTrackFlagsToString(uint8_t tracks);
  
};

} // namespace Vector
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_Components_AnimTrackHelpers_H__
