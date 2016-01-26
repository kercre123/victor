/** @file localization_geometry.h
* @author Daniel Casner
* Extracted geometry.h by Peter Barnum, 2013
* @date 2015
*
* Copyright Anki, Inc. 2015
* For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/
#include "localization_geometry.h"
#include <math.h>

namespace Anki {
  namespace Embedded {

    Point2f::Point2f(): x(0.0f), y(0.0f) {}
    Point2f::Point2f(const float x, const float y): x(x), y(y) {}
    Point2f::Point2f(const Point2f& pt): x(pt.x), y(pt.y) {}

    bool Point2f::operator== (const Point2f& other) const {
      return x == other.x && y == other.y;
    }

    Point2f& Point2f::operator*= (const float value) {
      x *= value;
      y *= value;
      return *this;
    }

    Point2f& Point2f::operator/= (const float value) {
      x /= value;
      y /= value;
      return *this;
    }

    Point2f& Point2f::operator+= (const Point2f& other) {
      x += other.x;
      y += other.y;
      return *this;
    }

    Point2f& Point2f::operator-= (const Point2f& other) {
      x -= other.x;
      y -= other.y;
      return *this;
    }

    Point2f& Point2f::operator= (const Point2f& other) {
      x = other.x;
      y = other.y;
      return *this;
    }

    float Point2f::Dist(const Point2f& other) const {
      const float xdiff = x - other.x;
      const float ydiff = y - other.y;
      return sqrtf(xdiff*xdiff + ydiff*ydiff);
    }

    float Point2f::Length() const {
      return sqrtf(x*x + y*y);
    }
  }
}
