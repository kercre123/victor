//
//  color.h
//  BaseStation
//
//  Created by Mark Pauley on 8/4/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#ifndef __Util_Color_Color_H__
#define __Util_Color_Color_H__

#include <json/json-forwards.h>
#include <cstdint>

// ANSI color escape codes
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

namespace Anki {
namespace Util {
  struct Color {
    Color() : _red(0), _green(0), _blue(0), _alpha(0) {};
    Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) : _red(red), _green(green), _blue(blue), _alpha(alpha) {};
    Color(float red, float green, float blue, float alpha = 1.0) : _red(red * 255), _green(green * 255), _blue(blue * 255), _alpha(alpha * 255) {};
    Color(double red, double green, double blue, double alpha = 1.0) : _red(red * 255), _green(green * 255), _blue(blue * 255), _alpha(alpha * 255) {};
    Color(const Json::Value& config);
    
    // return raw values
    float GetRed() const { return _red / 255.f; }
    float GetGreen() const { return _green / 255.f; }
    float GetBlue() const { return _blue / 255.f; }
    float GetAlpha() const { return _alpha / 255.f; }

    // return alpha adjusted values if placed on top of black
    float GetRedWithAlpha() const {return GetAlpha() * GetRed();}
    float GetGreenWithAlpha() const {return GetAlpha() * GetGreen();}
    float GetBlueWithAlpha() const {return GetAlpha() * GetBlue();}

    void SetAlpha(float alpha) {_alpha = alpha * 255;}
    void SetAlpha(uint8_t alpha) {_alpha = alpha;}
    
    //void ToPTree(boost::property_tree::ptree& ptree) const;
    void ToJSON(Json::Value& value) const;
    
    uint8_t _red;
    uint8_t _green;
    uint8_t _blue;
    uint8_t _alpha;
  };
  
  
} // end namespace Util
} // end namespace Anki


#endif /* defined(__Util_Color_Color_H__) */
