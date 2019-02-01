/**
* File: radians.cpp
*
* Author: Boris Sofman (boris)
* Created: 6/14/2008
*
* Description: Implementation of a radians class that automatically rescales
*              the value after any computation to be within (-PI, PI].
*
* Copyright: Anki, Inc. 2008-2011
*
**/

#include "coretech/common/shared/types.h"
#include "coretech/common/shared/math/radians.h"

#include <assert.h>
#include <cmath>
#include <cstdlib>

namespace Anki {
  //////////////////////////////////////////////////////////////////
  // Constructors
  //////////////////////////////////////////////////////////////////

  // Default constructor -- sets radian value to zero
  Radians::Radians()
  {
    radians_ = 0.0f;
    doRescaling_ = true;
  }

  // Constructs Radians object from input double (rescales accordingly)
  Radians::Radians(float initRad)
  {
    doRescaling_ = true;
    (*this) = initRad;
  }

  // Copy constructor (duplicates another Radians object)
  Radians::Radians(const Radians& initRad)
  {
    (*this) = initRad;
  }

  //////////////////////////////////////////////////////////////////
  // Arithmetic And Comparison Operators
  //////////////////////////////////////////////////////////////////

  // Angle addition operators
  Radians operator+(const Radians& a, const Radians& b)
  {
    Radians newRadians;

    newRadians.radians_ = a.radians_ + b.radians_;
    newRadians.rescale();

    return newRadians;
  }
  Radians operator+(const Radians& a, float b)
  {
    return a + Radians(b);
  }
  Radians operator+(float a, const Radians& b)
  {
    return Radians(a) + b;
  }
  void Radians::operator+=(float b)
  {
    (*this) = (*this) + b;
  }
  void Radians::operator+=(const Radians& b)
  {
    (*this) = (*this) + b;
  }

  // Angle subtraction operators
  Radians operator-(const Radians& a, const Radians& b)
  {
    Radians newRadians;

    newRadians.radians_ = a.radians_ - b.radians_;
    newRadians.rescale();

    return newRadians;
  }
  Radians operator-(const Radians& a, float b)
  {
    return a - Radians(b);
  }
  Radians operator-(float a, const Radians& b)
  {
    return Radians(a) - b;
  }
  void Radians::operator-=(float b)
  {
    (*this) = (*this) - b;
  }
  void Radians::operator-=(const Radians& b)
  {
    (*this) = (*this) - b;
  }

  // Negation operator
  Radians Radians::operator-() const
  {
    Radians newRadians;

    newRadians.radians_ = -radians_;
    if(doRescaling_) {
      newRadians.doRescaling_ = true;
      newRadians.rescale();
    }
    else {
      newRadians.doRescaling_ = false;
    }

    return newRadians;
  }

  // Angle multiplication operators
  Radians operator*(const Radians& a, float b)
  {
    Radians newRadians;

    newRadians.radians_ = a.radians_ * b;
    newRadians.rescale();

    return newRadians;
  }
  Radians operator*(float a, const Radians& b)
  {
    return b * a;
  }
  void Radians::operator*=(float b)
  {
    (*this) = (*this) * b;
  }

  // Angle division operator
  Radians operator/(const Radians& a, float b)
  {
    // Check for divide by 0
    assert(!NEAR_ZERO(b));  // Check for divide by 0

    Radians newRadians;
    newRadians.radians_ = a.radians_ / b;
    newRadians.rescale();

    return newRadians;
  }
  void Radians::operator/=(float b)
  {
    (*this) = (*this) / b;
  }

  // Equality operator: Returns true if the magnitude of difference between the
  // two radian values is within tolerance.
  bool operator==(const Radians& a, const Radians& b)
  {
    return a.IsNear(b);
  }

  // Inequality operator: Returns true if the magnitude of difference between
  // the two radian values is outside tolerance.
  bool operator!=(const Radians& a, const Radians& b)
  {
    return !(a == b);
  }

  // Returns true if a > b
  bool operator>(const Radians& a, const Radians& b)
  {
    return((a.ToFloat() - b.ToFloat()) > 0) && (a != b);
  }

  // Returns true if a >= b
  bool operator>=(const Radians& a, const Radians& b)
  {
    return(a > b) || (a == b);
  }

  // Returns true if a < b
  bool operator<(const Radians& a, const Radians& b)
  {
    return(b > a);
  }

  // Returns true if a <= b
  bool operator<=(const Radians& a, const Radians& b)
  {
    return(b >= a);
  }

  bool Radians::IsNear(const Radians& other, const Radians& epsilon) const
  {
    return std::abs((radians_ - other).ToFloat()) < std::abs(epsilon.ToFloat());
  }
  
  // Assignment operators
  void Radians::operator=(const Radians& b)
  {
    radians_ = b.radians_;
    doRescaling_ = b.doRescaling_;
  }
  void Radians::operator=(float b)
  {
    radians_ = b;
    rescale();
  }

  // Returns a radians object that's the absolute value of this one
  Radians Radians::getAbsoluteVal() const
  {
    return Radians(std::abs(radians_));
  }

  // Returns the object's radians value in degrees
  float Radians::getDegrees() const
  {
    return RAD_TO_DEG(radians_);
  }

  // Converts the passed argument from degrees to radians and sets the object's
  // value to the result
  void Radians::setDegrees(float degrees)
  {
    radians_ = DEG_TO_RAD(degrees);
    rescale();
  }

  void Radians::performRescaling(bool doRescaling)
  {
    doRescaling_ = doRescaling;
  }

  float Radians::angularDistance(const Radians& destAngle, bool clockwise) {
    // Get absolute difference
    float diff = destAngle.ToFloat() - radians_;

    // If clockwise then difference should be -ve, and vice versa.
    if(!clockwise && diff < 0.0f) {
      diff += 2.0f*M_PI_F;
    } else if(clockwise && diff > 0.0f) {
      diff -= 2.0f*M_PI_F;
    }

    return diff;
  }

  // Rescales the radians value of the object to be within (-PI, PI] range.
  void Radians::rescale()
  {
    int shiftAmt;

    if(!doRescaling_) {
      return;
    }

    // Check if already within range
    if(radians_ > -M_PI_F  && radians_ <= M_PI_F) {
      return;
    }

    if(std::abs(radians_) < 10.0f) {
      // For small values of radians, rescale manually to avoid doing division
      while(radians_ <= -M_PI_F) {
        radians_ += 2.0f*M_PI_F;
      }
      while(radians_ > M_PI_F) {
        radians_ -= 2.0f*M_PI_F;
      }
    } else {
      // Otherwise compute and adjust at once (more expensive due to divide)

      // If not, compute amount to shift by.  Divide by 2*M_PI_F but subtract .5 since
      // we're going to be in the (-PI, PI] range.
      shiftAmt = (int)ceilf((radians_ / (2.0f * M_PI_F)) - 0.5f);
      radians_ -= (2.0f * M_PI_F * (float)shiftAmt);
    }
  }

  ///////////////////////////////////////////////////////////////////////////////

  void Radians_noRescale::makePositive()
  {
    // Increase radians_ until it's positive
    while(radians_ < 0.0f) {
      radians_ += 2.0f * M_PI_F;
    }
  }

  void Radians_noRescale::makeNegative()
  {
    // Increase radians_ until it's negative
    while(radians_ > 0.0f) {
      radians_ -= 2.0f * M_PI_F;
    }
  }
} // namespace Anki
