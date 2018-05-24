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

#include "coretech/common/engine/math/quad.h"

/**
 * A class representing a rectangle in a planar (2D) environment. This
 * class provides a constructor that can create a rectangle out of the
 * parameters in a line in an environment file, or out of any convex
 * polygon. It will also automatically compute unit vectors as needed
 * and provides a checkPoint function to check if a given point is
 * inside the rectangle.
 *
 * NOTE: this class was designed for use in the planner. It uses extra
 * memory in exchange for faster collision checking. Take that into
 * consideration before creating thousands of these
 */
namespace Anki {

class RotatedRectangle
{
public:

  RotatedRectangle();

  /** This constructor builds a rectangle using two points and a
   * length. The two points define one side of the rectangle, and then
   * the length defines the other side, which is perpendicular to the
   * first.
   */
  RotatedRectangle(float x0, float y0, float x1, float y1, float otherSideLength);

  /** Given a convex polygon as a container of 2D float points, set
   * this object to be the roatated rectangle of minimum area which
   * bounds the polygon
   */
  explicit RotatedRectangle(const Quad2f& quad); // construct directly from quad
  void ImportQuad(const Quad2f& quad);

  /** Returns true if the given (x,y) point is inside (or on the
   * border of) this rectangle
   */
  bool Contains(float x, float y) const;

  /** Outputs in same format as input to the constructor, separated by
   * a space.
   */
  void Dump(std::ostream& out) const;

  float GetWidth() const {return std::abs(length0);}
  float GetHeight() const {return std::abs(length1);}
  float GetX() const {return cornerX;}
  float GetY() const {return cornerY;}

  Quad2f GetQuad() const;

private:

  // internal representation is a corner, two unit vectors (from that
  // corner) and two lengths

  float length0, length1;
  float cornerX, cornerY;
  float vec0X, vec0Y; // vec 0 points along given line (from point 1 to 0)
  float vec1X, vec1Y;

  void InitFromPoints(float x0, float y0, float x1, float y1, float otherSideLength);
};

}

#endif

