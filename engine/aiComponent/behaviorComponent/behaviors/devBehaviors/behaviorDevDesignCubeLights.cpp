/**
 * File: BehaviorDevDesignCubeLights.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2018-06-20
 *
 * Description: A behavior which exposes console vars to design cube lights
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevDesignCubeLights.h"

#include "engine/activeObject.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/wallTime.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

#if ANKI_DEV_CHEATS

#define CONSOLE_GROUP "CubeLightDesign"

#define MAX_SEG_LEN 7650

// LED 1
CONSOLE_VAR_RANGED(u8, kLED1_s1_red, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED1_s1_green, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED1_s1_blue, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED1_s1_alpha, CONSOLE_GROUP, 255, 0, 255);

CONSOLE_VAR_RANGED(u8, kLED1_s2_red, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED1_s2_green, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED1_s2_blue, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED1_s2_alpha, CONSOLE_GROUP, 255, 0, 255);

CONSOLE_VAR_RANGED(u32, kLED1_s1_hold_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN);
CONSOLE_VAR_RANGED(u32, kLED1_s2_hold_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN);
CONSOLE_VAR_RANGED(u32, kLED1_s1_transition_s2_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN);
CONSOLE_VAR_RANGED(u32, kLED1_s2_transition_s1_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN);
CONSOLE_VAR_RANGED(u32, kLED1_s1_hold_offset_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN*4);


// LED 2
CONSOLE_VAR_RANGED(u8, kLED2_s1_red, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED2_s1_green, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED2_s1_blue, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED2_s1_alpha, CONSOLE_GROUP, 255, 0, 255);

CONSOLE_VAR_RANGED(u8, kLED2_s2_red, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED2_s2_green, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED2_s2_blue, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED2_s2_alpha, CONSOLE_GROUP, 255, 0, 255);

CONSOLE_VAR_RANGED(u32, kLED2_s1_hold_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN);
CONSOLE_VAR_RANGED(u32, kLED2_s2_hold_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN);
CONSOLE_VAR_RANGED(u32, kLED2_s1_transition_s2_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN);
CONSOLE_VAR_RANGED(u32, kLED2_s2_transition_s1_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN);
CONSOLE_VAR_RANGED(u32, kLED2_s1_hold_offset_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN * 4);


// LED 3
CONSOLE_VAR_RANGED(u8, kLED3_s1_red, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED3_s1_green, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED3_s1_blue, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED3_s1_alpha, CONSOLE_GROUP, 255, 0, 255);

CONSOLE_VAR_RANGED(u8, kLED3_s2_red, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED3_s2_green, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED3_s2_blue, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED3_s2_alpha, CONSOLE_GROUP, 255, 0, 255);

CONSOLE_VAR_RANGED(u32, kLED3_s1_hold_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN);
CONSOLE_VAR_RANGED(u32, kLED3_s2_hold_ms, CONSOLE_GROUP, 0, 0,  MAX_SEG_LEN);
CONSOLE_VAR_RANGED(u32, kLED3_s1_transition_s2_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN);
CONSOLE_VAR_RANGED(u32, kLED3_s2_transition_s1_ms, CONSOLE_GROUP, 0, 0,  MAX_SEG_LEN);
CONSOLE_VAR_RANGED(u32, kLED3_s1_hold_offset_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN*4);


// LED 4
CONSOLE_VAR_RANGED(u8, kLED4_s1_red, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED4_s1_green, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED4_s1_blue, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED4_s1_alpha, CONSOLE_GROUP, 255, 0, 255);

CONSOLE_VAR_RANGED(u8, kLED4_s2_red, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED4_s2_green, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED4_s2_blue, CONSOLE_GROUP, 0, 0, 255);
CONSOLE_VAR_RANGED(u8, kLED4_s2_alpha, CONSOLE_GROUP,  255, 0, 255);

CONSOLE_VAR_RANGED(u32, kLED4_s1_hold_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN);
CONSOLE_VAR_RANGED(u32, kLED4_s2_hold_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN);
CONSOLE_VAR_RANGED(u32, kLED4_s1_transition_s2_ms, CONSOLE_GROUP, 0, 0,  MAX_SEG_LEN);
CONSOLE_VAR_RANGED(u32, kLED4_s2_transition_s1_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN);
CONSOLE_VAR_RANGED(u32, kLED4_s1_hold_offset_ms, CONSOLE_GROUP, 0, 0, MAX_SEG_LEN*4);


CONSOLE_VAR(bool, kRotate, CONSOLE_GROUP, false);

bool s_shouldSaveNextTick = false;

void SaveLights(ConsoleFunctionContextRef context)
{
  s_shouldSaveNextTick = true;
}

CONSOLE_FUNC(SaveLights, CONSOLE_GROUP);

#undef CONSOLE_GROUP

#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevDesignCubeLights::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevDesignCubeLights::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevDesignCubeLights::BehaviorDevDesignCubeLights(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevDesignCubeLights::~BehaviorDevDesignCubeLights()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDevDesignCubeLights::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevDesignCubeLights::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevDesignCubeLights::GetAllDelegates(std::set<IBehavior*>& delegates) const
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevDesignCubeLights::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevDesignCubeLights::OnBehaviorActivated() 
{
  _dVars = DynamicVariables();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevDesignCubeLights::BehaviorUpdate() 
{
  if(!IsActivated() ) {
    return;
  }

  #if ANKI_DEV_CHEATS
    auto newLights = BuildLightsFromConsoleVars();
    std::list<CubeLightAnimation::LightPattern> anim;
    CubeLightAnimation::LightPattern pattern;
    pattern.lights = newLights;
    anim.push_back(pattern);

    if(s_shouldSaveNextTick){
      struct tm time;
      WallTime::getInstance()->GetLocalTime(time);
      const std::string filename = "DevLights_" + std::to_string(time.tm_hour) + ":" + std::to_string(time.tm_min) + ":" + std::to_string(time.tm_sec);
      GetBEI().GetCubeLightComponent().SaveLightsToDisk(filename, anim);
      s_shouldSaveNextTick = false;
    }

    BlockWorldFilter filter;
    filter.AddAllowedFamily(ObjectFamily::LightCube);
    const ActiveObject* obj = GetBEI().GetBlockWorld().FindConnectedActiveMatchingObject(filter);

    if((newLights != _dVars.currentLights) &&
       (obj != nullptr)){
      auto debugStr = "DevDesignCubeLights";
      if(_dVars.handle == kInvalidHandle){
        GetBEI().GetCubeLightComponent().StopAllAnims();
        GetBEI().GetCubeLightComponent().PlayLightAnim(obj->GetID(), anim, {}, debugStr, _dVars.handle);
      }else{
        GetBEI().GetCubeLightComponent().StopAndPlayLightAnim(obj->GetID(), _dVars.handle, anim, debugStr);
      }
      _dVars.currentLights = newLights;
    }
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CubeLightAnimation::ObjectLights BehaviorDevDesignCubeLights::BuildLightsFromConsoleVars() const
{
  CubeLightAnimation::ObjectLights lights;
  #if ANKI_DEV_CHEATS
    // Setup state 1 for LEDs
    {
      ColorRGBA led1(kLED1_s1_red, kLED1_s1_green, kLED1_s1_blue, kLED1_s1_alpha);
      lights.onColors[0] = led1.AsRGBA();

      ColorRGBA led2(kLED2_s1_red, kLED2_s1_green, kLED2_s1_blue, kLED2_s1_alpha);
      lights.onColors[1] = led2.AsRGBA();

      ColorRGBA led3(kLED3_s1_red, kLED3_s1_green, kLED3_s1_blue, kLED3_s1_alpha);
      lights.onColors[2] = led3.AsRGBA();

      ColorRGBA led4(kLED4_s1_red, kLED4_s1_green, kLED4_s1_blue, kLED4_s1_alpha);
      lights.onColors[3] = led4.AsRGBA();
    }

    // Setup state 2 for LEDs
    {
      ColorRGBA led1(kLED1_s2_red, kLED1_s2_green, kLED1_s2_blue, kLED1_s2_alpha);
      lights.offColors[0] = led1.AsRGBA();

      ColorRGBA led2(kLED2_s2_red,kLED2_s2_green,  kLED2_s2_blue, kLED2_s2_alpha);
      lights.offColors[1] = led2.AsRGBA();

      ColorRGBA led3(kLED3_s2_red, kLED3_s2_green, kLED3_s2_blue, kLED3_s2_alpha);
      lights.offColors[2] = led3.AsRGBA();

      ColorRGBA led4(kLED4_s2_red, kLED4_s2_green, kLED4_s2_blue, kLED4_s2_alpha);
      lights.offColors[3] = led4.AsRGBA();
    }


    // Setup state 1 hold
    {
      lights.onPeriod_ms[0] = kLED1_s1_hold_ms;
      lights.onPeriod_ms[1] = kLED2_s1_hold_ms;
      lights.onPeriod_ms[2] = kLED3_s1_hold_ms;
      lights.onPeriod_ms[3] = kLED4_s1_hold_ms;
    }

    // Setup state 2 hold
    {
      lights.offPeriod_ms[0] = kLED1_s2_hold_ms;
      lights.offPeriod_ms[1] = kLED2_s2_hold_ms;
      lights.offPeriod_ms[2] = kLED3_s2_hold_ms;
      lights.offPeriod_ms[3] = kLED4_s2_hold_ms;
    }


    // Setup state 1 -> state 2 transition
    {
      lights.transitionOffPeriod_ms[0] = kLED1_s1_transition_s2_ms;
      lights.transitionOffPeriod_ms[1] = kLED2_s1_transition_s2_ms;
      lights.transitionOffPeriod_ms[2] = kLED3_s1_transition_s2_ms;
      lights.transitionOffPeriod_ms[3] = kLED4_s1_transition_s2_ms;
    }

    // Setup state 2 -> state 1 transition
    {
      lights.transitionOnPeriod_ms[0] = kLED1_s2_transition_s1_ms;
      lights.transitionOnPeriod_ms[1] = kLED2_s2_transition_s1_ms;
      lights.transitionOnPeriod_ms[2] = kLED3_s2_transition_s1_ms;
      lights.transitionOnPeriod_ms[3] = kLED4_s2_transition_s1_ms;
    }

    // Offsets for start
    {
      lights.offset[0] = kLED1_s1_hold_offset_ms;
      lights.offset[1] = kLED2_s1_hold_offset_ms;
      lights.offset[2] = kLED3_s1_hold_offset_ms;
      lights.offset[3] = kLED4_s1_hold_offset_ms;
    }

    lights.rotate = kRotate;
  #endif


  return lights;
}

}
}
