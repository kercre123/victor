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

#include "anki/common/basestation/colorRGBA.h"

namespace Anki {
  
  ColorRGBA::ColorRGBA()
  : ColorRGBA(DEFAULT)
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
                          "Float RGBA values should be on the interval [0,1]. Will clip.\n");
    }
  }
  
  ColorRGBA::ColorRGBA(u32 packedColor)
  : _color({{static_cast<u8>((0xFF000000 & packedColor) >> 24),
    static_cast<u8>((0x00FF0000 & packedColor) >> 16),
    static_cast<u8>((0x0000FF00 & packedColor) >>  8),
    static_cast<u8>((0x000000FF & packedColor))}})
  {

  }
  
  // Instantiate static const named colors
  const ColorRGBA ColorRGBA::RED       (1.f, 0.f, 0.f);
  const ColorRGBA ColorRGBA::GREEN     (0.f, 1.f, 0.f);
  const ColorRGBA ColorRGBA::BLUE      (0.f, 0.f, 1.f);
  const ColorRGBA ColorRGBA::YELLOW    (1.f, 1.f, 0.f);
  const ColorRGBA ColorRGBA::ORANGE    (1.f, .5f, 0.f);
  const ColorRGBA ColorRGBA::WHITE     (1.f, 1.f, 1.f);
  const ColorRGBA ColorRGBA::BLACK     (0.f, 0.f, 0.f);
  const ColorRGBA ColorRGBA::DEFAULT   (1.f, .8f, 0.f);
  const ColorRGBA ColorRGBA::DARKGRAY  (.3f, 0.3f, 0.3f);
  const ColorRGBA ColorRGBA::DARKGREEN (0.f, 0.5f, 0.0f);
  const ColorRGBA ColorRGBA::OFFWHITE  (0.f, 0.8f, 0.8f);

} // namespace Anki