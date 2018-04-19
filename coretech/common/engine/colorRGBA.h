/**
 * File: colorRGBA.h
 *
 * Author: Andrew Stein
 * Date:   8/8/2014
 *
 * Description:  Defines an object for defining a RGBA (Red, Green, Blue, Alpha)
 *               color, which will pack itself into a 32-bit integer for sending
 *               in messages.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_CORETECH_COMMON_COLORRGBA_H
#define ANKI_CORETECH_COMMON_COLORRGBA_H

#include "coretech/common/shared/types.h"

#include "util/logging/logging.h"

#include <array>

namespace Anki {

  class ColorRGBA
  {
  public:
    
    ColorRGBA(); // use static const DEFAULT
    ColorRGBA(u8  red, u8  green, u8  blue, u8  alpha = 255);
    ColorRGBA(f32 red, f32 green, f32 blue, f32 alpha = 1.f);
    ColorRGBA(u32 packedColor);
    
    // Accessors for u8 channels (as stored):
    u8 r() const;
    u8 g() const;
    u8 b() const;
    u8 alpha() const;
    
    u8& r();
    u8& g();
    u8& b();
    u8& alpha();
    
    // Accessors for f32 channels (computed):
    f32 GetR() const;
    f32 GetG() const;
    f32 GetB() const;
    f32 GetAlpha() const;
    
    void SetR(f32 r);
    void SetG(f32 g);
    void SetB(f32 b);
    void SetAlpha(f32 a);
    
    // Pack into a 32-bit integer [R G B A]
    u32 AsRGBA() const;
    
    // Packs into a 32-bit integer [0 R G B]
    u32 As0RGB() const;

    // Convenience conversion operator using pack function
    operator u32() const { return AsRGBA(); }
    
    static ColorRGBA CreateFromColorIndex(uint32_t colorIndex);
    
    // Convert string name to color
    
  protected:
    static constexpr f32 oneOver255 = 1.f / 255.f;
    
    static f32 GetF32(const u8 value);
    static u8  GetU8(const f32 value);
    
    std::array<u8, 4> _color;
    
  }; // class ColorRGBA
  
  namespace NamedColors {
    // Named Colors:
    // (Add others here and instantiate with actual r,g,b,a values in .cpp file)
    extern const ColorRGBA RED;
    extern const ColorRGBA GREEN;
    extern const ColorRGBA BLUE;
    extern const ColorRGBA YELLOW;
    extern const ColorRGBA CYAN;
    extern const ColorRGBA ORANGE;
    extern const ColorRGBA MAGENTA;
    extern const ColorRGBA WHITE;
    extern const ColorRGBA BLACK;
    extern const ColorRGBA DARKGRAY;
    extern const ColorRGBA LIGHTGRAY;
    extern const ColorRGBA DARKGREEN;
    extern const ColorRGBA OFFWHITE;
    extern const ColorRGBA PINK;
    extern const ColorRGBA DEFAULT;
    
    const ColorRGBA& GetByString(const std::string& name);
  } // namespace NamedColors
  
  
  inline u8 ColorRGBA::r() const { return _color[0]; }
  inline u8 ColorRGBA::g() const { return _color[1]; }
  inline u8 ColorRGBA::b() const { return _color[2]; }
  inline u8 ColorRGBA::alpha() const { return _color[3]; }
  
  inline u8& ColorRGBA::r() { return _color[0]; }
  inline u8& ColorRGBA::g() { return _color[1]; }
  inline u8& ColorRGBA::b() { return _color[2]; }
  inline u8& ColorRGBA::alpha() { return _color[3]; }
  
  // Accessors for f32 channels (computed):
  inline f32 ColorRGBA::GetF32(const u8 value)
  {
    const f32 retVal = static_cast<f32>(value) * oneOver255;
    return retVal;
  }
  
  inline u8 ColorRGBA::GetU8(const f32 value)
  {
    if(value < 0.f || value > 1.f) {
      PRINT_NAMED_WARNING("ColorRGBA.GetU8.OutOfRangeValue",
                          "Float RGBA values should be on the interval [0,1]. Will clip.");
    }
    const u8 retVal = static_cast<u8>(value*255.f);
    return retVal;
  }
  
  inline f32 ColorRGBA::GetR() const {  return GetF32(r()); }
  inline f32 ColorRGBA::GetG() const {  return GetF32(g()); }
  inline f32 ColorRGBA::GetB() const {  return GetF32(b()); }
  inline f32 ColorRGBA::GetAlpha() const {  return GetF32(alpha()); }
  
  inline void ColorRGBA::SetR(f32 red)   { r() = GetU8(red);   }
  inline void ColorRGBA::SetG(f32 green) { g() = GetU8(green); }
  inline void ColorRGBA::SetB(f32 blue)  { b() = GetU8(blue);  }
  inline void ColorRGBA::SetAlpha(f32 alphaNew) { alpha() = GetU8(alphaNew); }
  
  inline u32 ColorRGBA::AsRGBA() const
  {
    const u32 packedValue = ((static_cast<u32>(r()) << 24) |
                             (static_cast<u32>(g()) << 16) |
                             (static_cast<u32>(b()) <<  8) |
                             (static_cast<u32>(alpha())));
    
    return packedValue;
  }
  
  inline u32 ColorRGBA::As0RGB() const
  {
    const u32 packedValue = ((static_cast<u32>(r()) << 16) |
                             (static_cast<u32>(g()) <<  8) |
                             (static_cast<u32>(b())      ) );
    
    return packedValue;
  }

  
} // namespace Anki

#endif // ANKI_CORETECH_COMMON_COLORRGBA_H
