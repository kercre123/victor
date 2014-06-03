/**
 * File: rotatedRect.h
 *
 * Author: Brad Neuman
 * Created: 2014-06-03
 *
 * Description: A 2D rectangle with a rotation.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef _ANKICORETECH_COMMON_ROTATEDRECT_H_
#define _ANKICORETECH_COMMON_ROTATEDRECT_H_

/**
 * A class representing a rectangle in a planar (2D) environment. This
 * class provides a constructor that can create a rectangle out of the
 * parameters in a line in an environment file, or out of any convex
 * polygon. It will also automatically compute unit vectors as needed
 * and provides a checkPoint function to check if a given point is
 * inside the rectangle.
 */
namespace Anki {

class RotatedRectangle
{
public:

  /** This constructor builds a rectangle using two points and a
   * length. The two points define one side of the rectangle, and then
   * the length defines the other side, which is perpendicular to the
   * first.
   */
  RotatedRectangle(float x0, float y0, float x1, float y1, float otherSideLength);

  /** Returns true if the given (x,y) point is inside (or on the
   * border of) this rectangle
   */
  bool Contains(float x, float y) const;

private:

  // internal representation is a corner, two unit vectors (from that
  // corner) and two lengths

  float length0, length1;
  float cornerX, cornerY;
  float vec0X, vec0Y; // vec 0 points along given line (from point 1 to 0)
  float vec1X, vec1Y;

};

}

#endif

