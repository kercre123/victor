#include "rectangle.h"
#include <math.h>

namespace Anki {
namespace Planning {

Rectangle::Rectangle(float x0, float y0, float x1, float y1, float otherSideLength)
{
  length0 = sqrt(pow(x1 - x0, 2) + pow(y1 - y0, 2));
  length1 = otherSideLength;

  cornerX = x1;
  cornerY = y1;

  vec0X = (x0 - x1) / length0;
  vec0Y = (y0 - y1) / length0;

  // TODO:(bn) assert check using coretech common flt_near that the length is 1

  // To construct the vector for the next side, we construct a vector
  // perpendicular to side 0, then turn it into a unit vector
  vec1X = (y0 - y1) / length0;
  vec1Y = (x1 - x0) / length0;

  // TODO:(bn) assert here too
}

bool Rectangle::IsPointInside(float x, float y) const
{
  // given the corner point and unit vectors, we can take a dot
  // product of the vector from the corner to the given point. This
  // will give us a projection of the point onto each side of the
  // rectangle. If that value is within 0 and the length of that side
  // for both sides, then the point is inside the rectangle

  float testVecX = x - cornerX;
  float testVecY = y - cornerY;

  float proj0 = vec0X * testVecX + vec0Y * testVecY;
  if(proj0 < 0.0 || proj0 > length0)
    return false;

  float proj1 = vec1X * testVecX + vec1Y * testVecY;
  if(proj1 < 0.0 || proj1 > length1)
    return false;

  return true;
}


}
}
