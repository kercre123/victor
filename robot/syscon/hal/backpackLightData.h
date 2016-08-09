#ifndef __BACKPACK_LIGHT_DATA
#define __BACKPACK_LIGHT_DATA

namespace BackpackLights {
  // Light params when charging
  static const LightState charging[] = {
    {0, 0, 0, 0, 0, 0}, 
    {0x03e0, 0x0180, 20, 6, 20, 6}, 
    {0x03e0, 0x0180, 20, 6, 20, 6}, 
    {0x03e0, 0x0180, 20, 6, 20, 6}, 
    {0, 0, 0, 0, 0, 0} 
  };

  // Light when charged
  static const LightState charged[] = {
    {0, 0, 0, 0, 0, 0}, 
    {0x03e0 | 0x7c00, 0x0180 | 0x3c00, 10, 6, 10, 6}, 
    {0x03e0 | 0x7c00, 0x0180 | 0x3c00, 10, 6, 10, 6}, 
    {0x03e0 | 0x7c00, 0x0180 | 0x3c00, 10, 6, 10, 6}, 
    {0, 0, 0, 0, 0, 0} 
  };

  static const LightState sleeping[] = {
    { 0x0000, 0x0000, 34, 67, 17, 17 },
    { 0x0007, 0x0000, 34, 67, 17, 17 },
    { 0x0007, 0x0000, 34, 67, 17, 17 },
    { 0x0007, 0x0000, 34, 67, 17, 17 },
    { 0x0000, 0x0000, 34, 67, 17, 17 }
  };

  static const LightState off[] = {
    { 0x0000, 0x0000, 34, 67, 17, 17 },
    { 0x0000, 0x0000, 34, 67, 17, 17 },
    { 0x0000, 0x0000, 34, 67, 17, 17 },
    { 0x0000, 0x0000, 34, 67, 17, 17 },
    { 0x0000, 0x0000, 34, 67, 17, 17 }
  };

  static const LightState charger_oos[] = {
    {      0, 0x0000, 20,  0, 0, 0 },
    { 0x7C00, 0x0000, 20, 20, 0, 0 },
    { 0x7C00, 0x0000, 20, 20, 0, 0 },
    { 0x7C00, 0x0000, 20, 20, 0, 0 },
    {      0, 0x0000, 20,  0, 0, 0 }
  };

  static const LightState startup[] = {
    {0, 0, 0, 0, 0, 0}, 
    {0x03e0, 0x0200, 10, 10, 50, 50}, 
    {0, 0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0, 0}  
  };
}

#endif
