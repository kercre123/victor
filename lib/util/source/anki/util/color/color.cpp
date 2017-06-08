//
//  color.cpp
//  BaseStation
//
//  Created by Mark Pauley on 8/4/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#include "color.h"
#include <json/json.h>

/*
 * Example json for color config:
 *  ...
 *  "lights_color" = { "red" = 1.0, "green" = 0.0, "blue" = 0.0, "alpha" = 1.0 }, (empty components will be treated as 0.0)
 *  ...
 */
namespace Anki {
namespace Util {
  
Color::Color(const Json::Value& colorTree) :
  _red(colorTree["red"].asFloat() * 255),
  _green(colorTree["green"].asFloat() * 255),
  _blue(colorTree["blue"].asFloat() * 255),
  _alpha(colorTree["alpha"].asFloat() * 255)
  {
  }

  void Color::ToJSON(Json::Value& tree) const
  {
    tree = Json::objectValue;
    tree["red"] = _red / 255.f;
    tree["green"] = _green / 255.f;
    tree["blue"] = _blue / 255.f;
    tree["alpha"] = _alpha / 255.f;
  }

} // end namespace Util
} // end namespace Anki
