/**
 * File: radians.h
 *
 * Author: Boris Sofman (boris)
 * Created: 6/12/2008
 * 
 * Description: Implementation of a radians class that automatically rescales
 *              the value after any computation to be within (-PI, PI]. 
 *
 * Copyright: Anki, Inc. 2011
 *
 **/

#ifndef _ANKICORETECH_COMMON_RADIANS_H_
#define _ANKICORETECH_COMMON_RADIANS_H_

#include "util/math/math.h"

namespace Anki {

// A radians class that automatically rescales the value after any computation
// to be within (-PI, PI].
class Radians {

public:

  // Constructors
  Radians(); // Default constructor -- sets radian value to zero
  Radians(float initRad); // Constructs Radians object from input float (rescales accordingly)
  Radians(const Radians& initRad); // Copy constructor (duplicates another Radians object)


  // Arithmetic And Comparison Operators
  friend Radians operator+(const Radians& a, const Radians& b); // Addition
  friend Radians operator+(const Radians& a, float b);         // Addition
  friend Radians operator+(float a, const Radians& b);         // Addition
  void operator+=(float b);                                    // Addition
  void operator+=(const Radians& b);                            // Addition
  friend Radians operator-(const Radians& a, const Radians& b); // Subtraction
  friend Radians operator-(const Radians& a, float b);         // Subtraction
  friend Radians operator-(float a, const Radians& b);         // Subtraction
  void operator-=(float b);                                    // Subtraction
  void operator-=(const Radians& b);                            // Subtraction
  friend Radians operator*(const Radians& a, float b);         // Multiplication
  friend Radians operator*(float a, const Radians& b);         // Multiplication
  void operator*=(float b);                                    // Multiplication
  friend Radians operator/(const Radians& a, float b);         // Division
  void operator/=(float b);                                    // Division
  Radians operator-() const;                                    // Negation

  // Equality operator: Returns true if the magnitude of difference between the
  // two radian values is within tolerance.
  friend bool operator==(const Radians& a, const Radians& b);

  // Inequality operator: Returns true if the magnitude of difference between
  // the two radian values is outside tolerance.
  friend bool operator!=(const Radians& a, const Radians& b);

  // Returns true if a > b
  friend bool operator>(const Radians& a, const Radians& b);

  // Returns true if a >= b
  friend bool operator>=(const Radians& a, const Radians& b);

  // Returns true if a < b
  friend bool operator<(const Radians& a, const Radians& b);

  // Returns true if a <= b
  friend bool operator<=(const Radians& a, const Radians& b);

  // Returns true if other is within epsilon radians
  bool IsNear(const Radians& other, const Radians& epsilon = Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT) const;

  // Assignment operators
  void operator=(const Radians& b);
  void operator=(float b);


  // Operator to cast the radian to a double
  double ToDouble() const { return (double)radians_; };

  // Operator to cast the radian to a float
  float ToFloat() const { return radians_;};

  // Returns a radians object that's the absolute value of this one
  Radians getAbsoluteVal() const;

  // Returns the object's radians value in degrees
  float getDegrees() const;

  // Converts the passed argument from degrees to radians and sets the object's
  // value to the result
  void setDegrees(float degrees); 

  // Sets whether to perform rescaling or not
  void performRescaling(bool doRescaling);

  // Angular distance in radians to the destAngle in the specified direction.
  // Does not rescale.
  float angularDistance(const Radians& destAngle, bool clockwise);

protected:

  // Radians value of object (kept within range by rescale function)
  float radians_;

  // Whether to perform rescaling
  bool doRescaling_;

private:

  // Rescales the radians value of the object to be within (-PI, PI] range.
  void rescale();
};


// Alternate version of the Radians class that doesn't perform rescaling
// (otherwise equivalent)
class Radians_noRescale : public Radians {

public:

  // Constructors: automatically set rescaling to false
  Radians_noRescale() 
  : Radians()
  {
    doRescaling_ = false;
  }

  Radians_noRescale(float initRad) 
  : Radians(initRad)
  {
    doRescaling_ = false;
  }

  Radians_noRescale(const Radians& initRad) 
  : Radians(initRad)
  {
    doRescaling_ = false;
  }

  // Rescales by multiples of 2*PI until the value is either positive or
  // negative
  void makePositive();
  void makeNegative();
};

} // namespace Anki

#endif // _ANKICORETECH_COMMON_RADIANS_H_

