/**
 * File: colorRGBA.cpp
 *
 * Author: Andrew Stein
 * Date:   8/8/2014
 *
 * Description:  Implements an object for defining a RGBA (Red, Green, Blue, Alpha)
 *               color, which will pack itself into a 32-bit integer for sending
 *               in messages.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "coretech/common/engine/colorRGBA.h"

#include <map>
#include <string>

namespace Anki {
  
  ColorRGBA::ColorRGBA()
  : ColorRGBA(NamedColors::DEFAULT)
  {
    
  }
  
  ColorRGBA::ColorRGBA(u8 red, u8 green, u8 blue, u8 alpha)
  : _color({{red, green, blue, alpha}})
  {
    
  }
  
  ColorRGBA::ColorRGBA(f32 red, f32 green, f32 blue, f32 alpha)
  : _color({{GetU8(red), GetU8(green), GetU8(blue), GetU8(alpha)}})
  {
    if(red   < 0.f || red   > 1.f ||
       green < 0.f || green > 1.f ||
       blue  < 0.f || blue  > 1.f ||
       alpha < 0.f || alpha > 1.f)
    {
      PRINT_NAMED_WARNING("ColorRGBA.OutOfRangeValues",
                          "Float RGBA values should be on the interval [0,1]. Will clip.");
    }
  }
  
  ColorRGBA::ColorRGBA(u32 packedColor)
  : _color({{static_cast<u8>(packedColor >> 24),
    static_cast<u8>(0x000000FF & (packedColor >> 16)),
    static_cast<u8>(0x000000FF & (packedColor >>  8)),
    static_cast<u8>(0x000000FF & packedColor)}})
  {

  }
  
  // Instantiate const named colors
  namespace NamedColors
  {
    const ColorRGBA RED       (1.f, 0.f, 0.f);
    const ColorRGBA GREEN     (0.f, 1.f, 0.f);
    const ColorRGBA BLUE      (0.f, 0.f, 1.f);
    const ColorRGBA YELLOW    (1.f, 1.f, 0.f);
    const ColorRGBA CYAN      (0.f, 1.f, 1.f);
    const ColorRGBA ORANGE    (1.f, .5f, 0.f);
    const ColorRGBA MAGENTA   (1.f, 0.f, 1.f);
    const ColorRGBA WHITE     (1.f, 1.f, 1.f);
    const ColorRGBA BLACK     (0.f, 0.f, 0.f);
    const ColorRGBA DEFAULT   (1.f, .8f, 0.f);
    const ColorRGBA DARKGRAY  (.3f, 0.3f, 0.3f);
    const ColorRGBA LIGHTGRAY (0.7f, 0.7f, 0.7f);
    const ColorRGBA DARKGREEN (0.f, 0.5f, 0.0f);
    const ColorRGBA OFFWHITE  (0.8f, 0.8f, 0.8f);
    const ColorRGBA PINK      (1.0f, 0.078f, 0.58f);
    
    const ColorRGBA& GetByString(const std::string& name)
    {
      static const std::map<std::string, const ColorRGBA& > LUT = {
        {"RED",        RED},
        {"GREEN",      GREEN},
        {"BLUE",       BLUE},
        {"YELLOW",     YELLOW},
        {"CYAN",       CYAN},
        {"ORANGE",     ORANGE},
        {"MAGENTA",    MAGENTA},
        {"WHITE",      WHITE},
        {"BLACK",      BLACK},
        {"DEFAULT",    DEFAULT},
        {"DARKGRAY",   DARKGRAY},
        {"DARKGREEN",  DARKGREEN},
        {"OFFWHITE",   OFFWHITE},
      };
      
      auto result = LUT.find(name);
      if(result == LUT.end()) {
        PRINT_NAMED_WARNING("NamedColors.GetByString",
                            "Unknown color name '%s', returning default",
                            name.c_str());
        return DEFAULT;
      } else {
        return result->second;
      }
    } // GetByName()
    
  } // Namespace NamedColors
  
  
  ColorRGBA ColorRGBA::CreateFromColorIndex(uint32_t colorIndex)
  {
    // Generate distinct "primary-ish" colors for each index
    // Generates 1 of the 7 possible non-black binary RGB combos (R,G,B,RG,RB,GB,RGB) at decreasing intensities for higher indices
    
    const uint32_t kNumIntensityLevels = 5;
    const uint32_t kMaxIntensity  = 255;
    const uint32_t kMinIntesity   =  64; // used for "off", min "on" intensity is 1 step above this
    const uint32_t kIntensityStep = (kMaxIntensity - kMinIntesity) / kNumIntensityLevels;
    const uint32_t kMaxColorIndex = (7 * kNumIntensityLevels);
    
    // ensure colorIndex is in 0..((7*kNumIntensityLevels)-1) range
    colorIndex = colorIndex % kMaxColorIndex;
    
    const uint32_t intensity = kMaxIntensity - (kIntensityStep * (colorIndex / 7));
    const uint32_t colType = (colorIndex % 7);
    
    const uint32_t red   = ((colType - 2)  < 4) ? intensity : kMinIntesity;
    const uint32_t green = ((colType % 2) == 0) ? intensity : kMinIntesity;
    const uint32_t blue  = ((colType < 4)     ) ? intensity : kMinIntesity;
    
    const ColorRGBA color((u8)red, (u8)green, (u8)blue);
    
    return color;
  }

  
} // namespace Anki
