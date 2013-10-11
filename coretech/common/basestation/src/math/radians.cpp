/**
 * File: radians.cpp
 *
 * Author: Boris Sofman (boris)
 * Created: 6/14/2008
 * 
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 * 
 * Description: Implementation of a radians class that automatically rescales
 *              the value after any computation to be within (-PI, PI].  Since
 *              the object can be cast directly to the double, it can be used
 *              with any trigonometry functions.
 *
 * Copyright: Anki, Inc. 2008-2011
 *
 **/

#include "anki/common/basestation/general.h"

#include "anki/common/basestation/math/radians.h"

namespace Anki {

//////////////////////////////////////////////////////////////////
// Constructors
//////////////////////////////////////////////////////////////////

// Default constructor -- sets radian value to zero
Radians::Radians() 
{
  radians_ = 0.0;
  doRescaling_ = true;
}

// Constructs Radians object from input double (rescales accordingly)
Radians::Radians(double initRad)
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
Radians operator+(const Radians& a, double b)
{
  return a + Radians(b);
}
Radians operator+(double a, const Radians& b)
{
  return Radians(a) + b;
}
void Radians::operator+=(double b)
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
Radians operator-(const Radians& a, double b)
{
  return a - Radians(b);
}
Radians operator-(double a, const Radians& b)
{
  return Radians(a) - b;
}
void Radians::operator-=(double b)
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
Radians operator*(const Radians& a, double b)
{
  Radians newRadians;

  newRadians.radians_ = a.radians_ * b;
  newRadians.rescale();

  return newRadians;
}
Radians operator*(double a, const Radians& b)
{
  return b * a;
}
void Radians::operator*=(double b)
{
  (*this) = (*this) * b;
}


// Angle division operator
Radians operator/(const Radians& a, double b)
{
  Radians newRadians;

  // Check for divide by 0
  if (NEAR_ZERO(b)) {
    PRINT_NAMED_ERROR("Radians", "Attempting to divide by 0!");
  }

  newRadians.radians_ = a.radians_ / b;
  newRadians.rescale();

  return newRadians;
}
void Radians::operator/=(double b)
{
  (*this) = (*this) / b;
}


// Equality operator: Returns true if the magnitude of difference between the
// two radian values is within tolerance.
bool operator==(const Radians& a, const Radians& b)
{
  return NEAR(a.radians_, b.radians_, FLOATING_POINT_COMPARISON_TOLERANCE);
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
  return((a.ToDouble() - b.ToDouble()) > 0) && (a != b);
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


// Assignment operators
void Radians::operator=(const Radians& b)
{
  radians_ = b.radians_;
  doRescaling_ = b.doRescaling_;
}
void Radians::operator=(double b)
{
  radians_ = b;
  rescale();
}


// Returns a radians object that's the absolute value of this one
Radians Radians::getAbsoluteVal()
{
  return Radians(ABS(radians_));
}


// Returns the object's radians value in degrees
double Radians::getDegrees() const 
{
  return RAD_TO_DEG(radians_);
}

// Converts the passed argument from degrees to radians and sets the object's
// value to the result
void Radians::setDegrees(double degrees)
{
  radians_ = DEG_TO_RAD(degrees);
  rescale();
}


void Radians::performRescaling(bool doRescaling)
{
  doRescaling_ = doRescaling;
}

double Radians::angularDistance(const Radians& destAngle, bool clockwise) {
  
  // Get absolute difference
  double diff = destAngle.ToDouble() - radians_;

  // If clockwise then difference should be -ve, and vice versa.
  if(!clockwise && diff < 0) {
    diff += 2.0*PI; 
  } else if(clockwise && diff > 0) {
    diff -= 2.0*PI;
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
  if(radians_ > -PI  && radians_ <= PI) {
    return;
  }


  if(ABS(radians_) < 10) {
    // For small values of radians, rescale manually to avoid doing division
    while(radians_ <= -PI) {
      radians_ += 2.0*PI;
    }
    while(radians_ > PI) {
      radians_ -= 2.0*PI;
    }
  } else {

    // Otherwise compute and adjust at once (more expensive due to divide)

    // If not, compute amount to shift by.  Divide by 2*PI but subtract .5 since
    // we're going to be in the (-PI, PI] range.
    shiftAmt = (int)ceil((radians_ / (2.0 * PI)) - 0.5);
    radians_ -= (2.0 * PI * (double)shiftAmt);
  }

}


///////////////////////////////////////////////////////////////////////////////

void Radians_noRescale::makePositive()
{
  // Increase radians_ until it's positive
  while(radians_ < 0) {
    radians_ += 2.0 * PI;
  }
}

void Radians_noRescale::makeNegative()
{
  // Increase radians_ until it's negative
  while(radians_ > 0) {
    radians_ -= 2.0 * PI;
  }
}


} // namespace Anki

