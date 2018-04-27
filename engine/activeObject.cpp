/**
 * File: ActiveObject.cpp
 *
 * Author: Kevin Yoon
 * Date:   5/19/2016
 *
 * Description: Defines an active object (i.e. one that has a radio connection and has lights that can be set)
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "engine/activeObject.h"
#include "engine/activeCube.h"
#include "engine/charger.h"

#define SAVE_SET_BLOCK_LIGHTS_MESSAGES_FOR_DEBUG 0

#if SAVE_SET_BLOCK_LIGHTS_MESSAGES_FOR_DEBUG 
  #include <fstream>
#endif

namespace Anki {
namespace Cozmo {

// The physical blocks are not capable of displaying
// all light channels at full intensity so this is where
// we compute the gamma scaler based on the desired light pattern.
void ActiveObject::RecomputeGamma()
{
  // TODO: To do this right, we need to actually simulate the animation cycle here and
  //       get the peak value of L_t for all values of time t in the cycle where
  //
  //          L_t = maxRed_t*maxRed_t + maxGreen_t*maxGreen_t + maxBlue_t*maxBlue_t
  //
  //       and max<color> is the maximum value of a particular channel across all LEDs.
  //       This value L_max needs to be less than 0xffff
  //
  //       Nathan says a simpler formula is to make sure
  //
  //          maxRed + maxGreen + maxBlue < 0xff
  //
  _ledGamma = 0x80;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActiveObject::SetLEDs(const WhichCubeLEDs whichLEDs,
                         const ColorRGBA& onColor,
                         const ColorRGBA& offColor,
                         const u32 onPeriod_ms,
                         const u32 offPeriod_ms,
                         const u32 transitionOnPeriod_ms,
                         const u32 transitionOffPeriod_ms,
                         const s32 offset,
                         const bool turnOffUnspecifiedLEDs)
{
  static const u8 FIRST_BIT = 0x01;
  u8 shiftedLEDs = static_cast<u8>(whichLEDs);
  for(u8 iLED=0; iLED<NUM_LEDS; ++iLED) {
    // If this LED is specified in whichLEDs (its bit is set), then
    // update
    if(shiftedLEDs & FIRST_BIT) {
      _ledState[iLED].onColor      = onColor;
      _ledState[iLED].offColor     = offColor;
      _ledState[iLED].onPeriod_ms  = onPeriod_ms;
      _ledState[iLED].offPeriod_ms = offPeriod_ms;
      _ledState[iLED].transitionOnPeriod_ms = transitionOnPeriod_ms;
      _ledState[iLED].transitionOffPeriod_ms = transitionOffPeriod_ms;
      _ledState[iLED].offset = offset;
    } else if(turnOffUnspecifiedLEDs) {
      _ledState[iLED].onColor      = ::Anki::NamedColors::BLACK;
      _ledState[iLED].offColor     = ::Anki::NamedColors::BLACK;
      _ledState[iLED].onPeriod_ms  = 1000;
      _ledState[iLED].offPeriod_ms = 1000;
      _ledState[iLED].transitionOnPeriod_ms = 0;
      _ledState[iLED].transitionOffPeriod_ms = 0;
      _ledState[iLED].offset = 0;
    }
    shiftedLEDs = shiftedLEDs >> 1;
  }
  RecomputeGamma();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActiveObject::SetLEDs(const std::array<u32,NUM_LEDS>& onColors,
                         const std::array<u32,NUM_LEDS>& offColors,
                         const std::array<u32,NUM_LEDS>& onPeriods_ms,
                         const std::array<u32,NUM_LEDS>& offPeriods_ms,
                         const std::array<u32,NUM_LEDS>& transitionOnPeriods_ms,
                         const std::array<u32,NUM_LEDS>& transitionOffPeriods_ms,
                         const std::array<s32,NUM_LEDS>& offsets)
{
  for(u8 iLED=0; iLED<NUM_LEDS; ++iLED) {
    _ledState[iLED].onColor      = onColors[iLED];
    _ledState[iLED].offColor     = offColors[iLED];
    _ledState[iLED].onPeriod_ms  = onPeriods_ms[iLED];
    _ledState[iLED].offPeriod_ms = offPeriods_ms[iLED];
    _ledState[iLED].transitionOnPeriod_ms = transitionOnPeriods_ms[iLED];
    _ledState[iLED].transitionOffPeriod_ms = transitionOffPeriods_ms[iLED];
    _ledState[iLED].offset = offsets[iLED];
  }
  RecomputeGamma();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActiveObject::CanBeUsedForLocalization() const
{
  if (IsPoseStateKnown() && IsMoving()) {
    // This shouldn't happen!
    PRINT_NAMED_WARNING("ActiveObject.CanBeUsedForLocalization.PoseStateKnownButMoving", "");
    return false;
  }
  
  return (GetPoseState() == PoseState::Known &&
          GetActiveID() >= 0 &&
          GetLastPoseUpdateDistance() >= 0.f &&
          IsRestingFlat(DEG_TO_RAD(GetRestingFlatTolForLocalization_deg())));
}

/*
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActiveObject::TurnOffAllLEDs()
{
  SetLEDs(WhichCubeLEDs::ALL, NamedColors::BLACK, 0, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActiveObject::FillMessage(MessageSetBlockLights& m) const
{
  m.blockID = _activeID;
  
  assert(m.onPeriod_ms.size() == NUM_LEDS);
  assert(m.offPeriod_ms.size() == NUM_LEDS);
  assert(m.onColor.size() == NUM_LEDS);
  assert(m.offColor.size() == NUM_LEDS);
  
  for(u8 iLED=0; iLED<NUM_LEDS; ++iLED) {
    m.onColor[iLED] = _ledState[iLED].onColor;
    m.offColor[iLED] = _ledState[iLED].offColor;
    m.onPeriod_ms[iLED]  = _ledState[iLED].onPeriod_ms;
    m.offPeriod_ms[iLED] = _ledState[iLED].offPeriod_ms;
    m.transitionOnPeriod_ms[iLED]  = _ledState[iLED].transitionOnPeriod_ms;
    m.transitionOffPeriod_ms[iLED] = _ledState[iLED].transitionOffPeriod_ms;
  }
  
#     if SAVE_SET_BLOCK_LIGHTS_MESSAGES_FOR_DEBUG
  {
    static int saveCtr=0;
    Json::Value jsonMsg = m.CreateJson();
    std::ofstream jsonFile("SetBlockLights_" + std::to_string(saveCtr++) + ".json", std::ofstream::out);
    fprintf(stdout, "Writing SetBlockLights message to JSON file.\n");
    jsonFile << jsonMsg.toStyledString();
    jsonFile.close();
  }
#     endif 
  
}
*/
  
} // namespace Cozmo
} // namespace Anki

