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

#include "anki/cozmo/basestation/activeObject.h"

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
    

    // TEMP (Kevin): WhiteBalancing is eventually to be done in body so just doing something simple here to get us by.
    //               Basically if there is any red at all, then blue and green channels are scaled down to 60%.
    void ActiveObject::DoWhiteBalancing()
    {
      for(auto iLED=0; iLED<NUM_LEDS; ++iLED) {
        
        if(_ledState[iLED].onColor.GetR() > 0) {
          _ledState[iLED].onColor.SetB( 0.6f * _ledState[iLED].onColor.GetB());
          _ledState[iLED].onColor.SetG( 0.6f * _ledState[iLED].onColor.GetG());
        }

        if(_ledState[iLED].offColor.GetR() > 0) {
          _ledState[iLED].offColor.SetB( 0.6f * _ledState[iLED].offColor.GetB());
          _ledState[iLED].offColor.SetG( 0.6f * _ledState[iLED].offColor.GetG());
        }
        
      }
    }

    
    void ActiveObject::SetLEDs(const WhichCubeLEDs whichLEDs,
                             const ColorRGBA& onColor,
                             const ColorRGBA& offColor,
                             const u32 onPeriod_ms,
                             const u32 offPeriod_ms,
                             const u32 transitionOnPeriod_ms,
                             const u32 transitionOffPeriod_ms,
                             const u32 onOffset,
                             const u32 offOffset,
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
          _ledState[iLED].onOffset = onOffset;
          _ledState[iLED].offOffset = offOffset;
        } else if(turnOffUnspecifiedLEDs) {
          _ledState[iLED].onColor      = ::Anki::NamedColors::BLACK;
          _ledState[iLED].offColor     = ::Anki::NamedColors::BLACK;
          _ledState[iLED].onPeriod_ms  = 1000;
          _ledState[iLED].offPeriod_ms = 1000;
          _ledState[iLED].transitionOnPeriod_ms = 0;
          _ledState[iLED].transitionOffPeriod_ms = 0;
          _ledState[iLED].onOffset = 0;
          _ledState[iLED].offOffset = 0;
        }
        shiftedLEDs = shiftedLEDs >> 1;
      }
      RecomputeGamma();
      DoWhiteBalancing();
    }
    
    void ActiveObject::SetLEDs(const std::array<u32,NUM_LEDS>& onColors,
                             const std::array<u32,NUM_LEDS>& offColors,
                             const std::array<u32,NUM_LEDS>& onPeriods_ms,
                             const std::array<u32,NUM_LEDS>& offPeriods_ms,
                             const std::array<u32,NUM_LEDS>& transitionOnPeriods_ms,
                             const std::array<u32,NUM_LEDS>& transitionOffPeriods_ms,
                             const std::array<u32,NUM_LEDS>& onOffsets,
                             const std::array<u32,NUM_LEDS>& offOffsets)
    {
      for(u8 iLED=0; iLED<NUM_LEDS; ++iLED) {
        _ledState[iLED].onColor      = onColors[iLED];
        _ledState[iLED].offColor     = offColors[iLED];
        _ledState[iLED].onPeriod_ms  = onPeriods_ms[iLED];
        _ledState[iLED].offPeriod_ms = offPeriods_ms[iLED];
        
        // Handle some special cases (we want to avoid on/off times of 0 for the
        // sake of the real active blocks)
        if(onPeriods_ms[iLED] == 0 && offPeriods_ms[iLED] > 0) {
          // Looks like we mean for this LED to be solid "off" color
          _ledState[iLED].onColor = offColors[iLED];
          _ledState[iLED].onPeriod_ms = u32_MAX/2;
        }
        else if(offPeriods_ms[iLED] == 0 && onPeriods_ms[iLED] > 0) {
          // Looks like we mean for this LED to be solid "on" color
          _ledState[iLED].offColor = onColors[iLED];
          _ledState[iLED].offPeriod_ms = u32_MAX/2;
        }
        else if(onPeriods_ms[iLED]==0 && offPeriods_ms[iLED]==0) {
          // Looks like we mean for this LED to actually turn off
          _ledState[iLED].onColor = 0;
          _ledState[iLED].offColor = 0;
          _ledState[iLED].onPeriod_ms = u32_MAX/2;
          _ledState[iLED].offPeriod_ms = u32_MAX/2;
        }
        
        _ledState[iLED].transitionOnPeriod_ms = transitionOnPeriods_ms[iLED];
        _ledState[iLED].transitionOffPeriod_ms = transitionOffPeriods_ms[iLED];
        _ledState[iLED].onOffset = onOffsets[iLED];
        _ledState[iLED].offOffset = offOffsets[iLED];
      }
      RecomputeGamma();
      DoWhiteBalancing();
    }
    
    
    bool ActiveObject::CanBeUsedForLocalization() const
    {
      return (GetPoseState() == PoseState::Known &&
              GetActiveID() >= 0 &&
              GetLastPoseUpdateDistance() >= 0.f &&
              GetIdentityState() == ActiveIdentityState::Identified &&
              IsRestingFlat(DEG_TO_RAD(5))); // TODO: AS: 5 was the old value, not sure if we still want it here
    }
    
    /*
    void ActiveObject::TurnOffAllLEDs()
    {
      SetLEDs(WhichCubeLEDs::ALL, NamedColors::BLACK, 0, 0);
    }

    
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

    
    ObjectType ActiveObject::GetTypeFromActiveObjectType(ActiveObjectType type) {
      ObjectType objType = ObjectType::Unknown;
      switch(type) {
        case ActiveObjectType::OBJECT_CHARGER:
          objType = ObjectType::Charger_Basic;
          break;
        case ActiveObjectType::OBJECT_CUBE1:
          objType = ObjectType::Block_LIGHTCUBE1;
          break;
        case ActiveObjectType::OBJECT_CUBE2:
          objType = ObjectType::Block_LIGHTCUBE2;
          break;
        case ActiveObjectType::OBJECT_CUBE3:
          objType = ObjectType::Block_LIGHTCUBE3;
          break;
        default:
          break;
      }
      
      return objType;
    }

  } // namespace Cozmo
} // namespace Anki