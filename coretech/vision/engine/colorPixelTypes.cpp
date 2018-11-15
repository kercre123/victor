/**
 * File: colorPixelTypes.cpp
 *
 * Author: Patrick Doran
 * Date:   10/15/2018
 *
 * Description: Source file for color Pixel types
 *
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "colorPixelTypes.h"

namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PixelHSV::FromPixelRGB(const PixelRGB& rgb)
{
  float min = rgb.min()/255.f;
  float max = rgb.max()/255.f;
  float& hue = h();
  float& sat = s();
  float& val = v();

  val = max;
  float delta = max - min;
  if (Util::IsNearZero(delta)) // division by 0 guard
  {
    sat = 0.f;
    hue = 0.f;
    return;
  }

  const float red = rgb[0]/255.f;
  const float green = rgb[1]/255.f;
  const float blue = rgb[2]/255.f;

  // Divide by 0 protection
  if (Util::IsNearZero(delta)){
    sat = 0.f;
    hue = 0.f; // technically, this is undefined if there is no saturation
  } else {
    sat = delta/max;
  }

  if (Util::IsFltNear(red,max)){
    hue = (green - blue)/delta;       //  yellow <-> magenta
  } else if (Util::IsFltNear(green,max)){
    hue = 2 + (blue - red) / delta;   //    cyan <-> yellow
  } else {
    hue = 4 + (red - green) / delta;  // magenta <-> cyan
  }
  hue *= 60;
  if ( hue < 0 ){
    hue += 360;
  }
  hue /= 360.f;
  Util::Clamp(hue,0.f,1.f);
  Util::Clamp(sat,0.f,1.f);
  Util::Clamp(val,0.f,1.f);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PixelRGB PixelHSV::ToPixelRGB() const
{
  float hue = h()*360.f;
  float sat = s();
  float val = v();

  hue /= 60.f;
  s32 index = floor(hue);          // sector of hue space, [0-5]
  float factorial = hue - index;   // factorial part of hue
  float p = val * (1 - sat);
  float q = val * (1 - sat * factorial);
  float t = val * (1 - sat * (1 - factorial));

  // Convert values from [0,1] to [0,255]
  val = static_cast<u8>(std::roundf(255.f * Util::Clamp(val, 0.f, 1.f)));
  p =   static_cast<u8>(std::roundf(255.f * Util::Clamp(  p, 0.f, 1.f)));
  q =   static_cast<u8>(std::roundf(255.f * Util::Clamp(  q, 0.f, 1.f)));
  t =   static_cast<u8>(std::roundf(255.f * Util::Clamp(  t, 0.f, 1.f)));

  switch (index){
    case 0:  return PixelRGB(val,   t,   p);
    case 1:  return PixelRGB(  q, val,   p);
    case 2:  return PixelRGB(  p, val,   t);
    case 3:  return PixelRGB(  p,   q, val);
    case 4:  return PixelRGB(  t,   p, val);
    default: return PixelRGB(val,   p,   q);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PixelYUV::FromPixelRGB(const PixelRGB& rgb)
{
  // ITU.BT-601 Y'CbCr conversions
  // NOTE: In 8-bit values, YCbCr fall into certain valid ranges:
  //    Y:      [16,235]
  //    Cb/Cr:  [16,240]
  // However, we use the whole 8-bit space [0,255] to represent the values. This has the benefit that it is
  // compatible with the YCbCr format defined by JPEG compression.

  y() = static_cast<u8>(Util::Clamp(roundf( 0.299f*rgb.r() + 0.587f*rgb.g() + 0.114f*rgb.b()),         0.f, 255.f));
  u() = static_cast<u8>(Util::Clamp(roundf(-0.169f*rgb.r() - 0.331f*rgb.g() + 0.500f*rgb.b() + 128.f), 0.f, 255.f));
  v() = static_cast<u8>(Util::Clamp(roundf( 0.500f*rgb.r() - 0.419f*rgb.g() - 0.081f*rgb.b() + 128.f), 0.f, 255.f));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PixelRGB PixelYUV::ToPixelRGB() const
{

  u8 r = static_cast<u8>(Util::Clamp(roundf(y()                        + 1.402f*(v()+128.f)),   0.f, 255.f));
  u8 g = static_cast<u8>(Util::Clamp(roundf(y() - 0.34414f*(u()-128.f) - 0.71414f*(v()-128.f)), 0.f, 255.f));
  u8 b = static_cast<u8>(Util::Clamp(roundf(y() + 1.772f  *(u()-128.f)),                        0.f, 255.f));

  return PixelRGB(r,g,b);
}

} /* namespace Vision */
} /* namespace Anki */
