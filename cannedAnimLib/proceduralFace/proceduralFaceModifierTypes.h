/**
 * File: proceduralFaceModifierTypes.h
 *
 * Authors: Jordan Rivas
 * Created: 05/09/2018
 *
 * Description: Define types for procedural face activities
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Anki_Cozmo_ProceduralFaceModifierTypes_H__
#define __Anki_Cozmo_ProceduralFaceModifierTypes_H__

#include <vector>

namespace Anki {
namespace Cozmo {

  // Procedural Eye Blink types
  enum class BlinkState : uint8_t {
    Closing,
    Closed,
    JustOpened,
    Opening
  };
  
  struct BlinkEvent {
    uint32_t Time_ms;
    BlinkState State;
    BlinkEvent(uint32_t time_ms, BlinkState state)
    : Time_ms(time_ms)
    , State(state) {}
  };
  
  using BlinkEventList = std::vector<BlinkEvent>;

} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_ProceduralFaceModifierTypes_H__ */
