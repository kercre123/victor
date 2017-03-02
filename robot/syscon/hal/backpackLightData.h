#ifndef __BACKPACK_LIGHT_DATA
#define __BACKPACK_LIGHT_DATA

namespace BackpackLights {
  static const LightState all_off[] = {
    { 0x0000, 0x0000 },
    { 0x0000, 0x0000 },
    { 0x0000, 0x0000 },
    { 0x0000, 0x0000 },
    { 0x0000, 0x0000 }
  };
  
  static const LightState charging[] = {
    { 0x01E0, 0x0000, 10, 30, 10,  5, -30 },
    { 0x01E0, 0x0000, 30, 10, 10,  5, -10 },
    { 0x01E0, 0x01E0 }
  };

  static const LightState charged[] = {
    { 0x01E0, 0x01E0 },
    { 0x01E0, 0x01E0 },
    { 0x01E0, 0x01E0 }
  };

  static const LightState button_pressed[] = {
    { 0x3C00, 0x3C00 },
    { 0x3C00, 0x3C00 },
    { 0x3C00, 0x3C00 }
  };

  static const LightState low_battery[] = {
    { 0x0000, 0x0000 },
    { 0x0000, 0x0000 },
    { 0x3C00, 0x0000, 20, 20, 10, 10,  0 }
  };

  static const LightState disconnected[] = {
    { 0x1ce7, 0x1ce7 },
    { 0x1ce7, 0x1ce7 },
    { 0x1ce7, 0x1ce7 }
  };
}

#endif
