/**
 * File: rotatedRect.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-06-03
 *
 * Description: A 2D rectangle with a rotation.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/rotatedRect.h"
#include "coretech/common/shared/math/rotation.h"
#include <math.h>

namespace Anki {

namespace RotatedRectHelper {

// helper function which tries to construct a bounding rectangle
// along the given edge. If the area is less than the given minArea,
// update min area and set this object to the current best rectangle
bool CheckBoundingRect(const std::vector<Point2f>& polygon,
                           size_t startIndex,
                           float& minArea,
                           Rectangle<float>& bestRect,
                           float& bestTheta);

}


RotatedRectangle::RotatedRectangle()
  : length0(0.0)
  , length1(0.0)
  , cornerX(0.0)
  , cornerY(0.0)
  , vec0X(0.0)
  , vec0Y(0.0)
  , vec1X(0.0)
  , vec1Y(0.0)
{
}

RotatedRectangle::RotatedRectangle(float x0, float y0, float x1, float y1, float otherSideLength)
{
  InitFromPoints(x0, y0, x1, y1, otherSideLength);
}
  
RotatedRectangle::RotatedRectangle(const Quad2f& quad)
{
  ImportQuad(quad);
}

void RotatedRectangle::InitFromPoints(float x0, float y0, float x1, float y1, float otherSideLength)
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

bool RotatedRectangle::Contains(float x, float y) const
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

Quad2f RotatedRectangle::GetQuad() const
{
  Point2f vec0(vec0X, vec0Y);
  Point2f vec1(vec1X, vec1Y);

  Point2f p0(cornerX, cornerY);
  Point2f p1 = p0 + vec0 * length0;
  Point2f p2 = p0 + vec0 * length0 + vec1 * length1;
  Point2f p3 = p0 + vec1 * length1;

  Quad2f Q(p0, p1, p2, p3);
  return Q.SortCornersClockwise();
}


void RotatedRectangle::ImportQuad(const Quad2f& quadUnsorted)
{
  Quad2f quad  = quadUnsorted.SortCornersClockwise();

  std::vector<Point2f> polygon = {quad[Quad::TopLeft],
                                 quad[Quad::TopRight],
                                 quad[Quad::BottomRight],
                                 quad[Quad::BottomLeft]};

  float minArea = std::numeric_limits<float>::infinity();
  Rectangle<float> bestRect;
  size_t bestIdx = 0;
  float bestTheta = 0.0;
  bool found = false;
  size_t size = polygon.size();

  if(size > 1) {
    for(size_t i=0; i<size; ++i) {
      if(RotatedRectHelper::CheckBoundingRect(polygon, i, minArea, bestRect, bestTheta)) {
        found = true;
        bestIdx = i;
      }
    }
  }

  // TODO:(bn) error handling!!
  if(found) {
    // bestRect (x,y) is the top left of an axis-aligned rectangle

    // I need two points for the rectangle definition
    Point2f point0(bestRect.GetWidth() - bestRect.GetX(), 0.0);
    Point2f point1(bestRect.GetX(), 0.0);

    // rotate and translate the points back into the frame of the
    // original polygon
    RotationMatrix2d rot(bestTheta);
    point0 = rot * point0 + polygon[bestIdx];
    point1 = rot * point1 + polygon[bestIdx];

    InitFromPoints(point1.x(), point1.y(),
                       point0.x(), point0.y(),
                       bestRect.GetHeight());


  }
}


bool RotatedRectHelper::CheckBoundingRect(const std::vector<Point2f>& polygon,
                                              size_t startIndex,
                                              float& minArea,
                                              Rectangle<float>& bestRect,
                                              float& bestTheta)
{
  // This function will check the rectangle which is aligned with the
  // line between start and end.
  const float x0(polygon[startIndex].x());
  const float y0(polygon[startIndex].y());

  size_t size = polygon.size();
  size_t endIndex = (startIndex + 1) % size;

  const float x1(polygon[endIndex].x());
  const float y1(polygon[endIndex].y());

  // rotate the points in the polygon so that the line is along the x-axis
  float theta = atan2(y1 - y0, x1 - x0);
  RotationMatrix2d rot(-theta);

  // rotate the quadrilateral, then construct a Rectangle from
  // it.
  auto rotatedPolygon(polygon);
  for(auto & point : rotatedPolygon) {
    point = rot * (point - polygon[startIndex]);
  }
  Rectangle<float> boundingRect(rotatedPolygon);

  // if the area of the rectangle is less than minArea, update minArea
  // and bestRect
  float area = boundingRect.Area();
  if(area < minArea) {
    minArea = area;
    bestRect = boundingRect;
    bestTheta = theta;
    return true;
  }

  return false;
}

void RotatedRectangle::Dump(std::ostream& out) const
{
  float x1 = cornerX;
  float y1 = cornerY;

  float x0 = x1 - vec1Y * length0;
  float y0 = vec1X * length0 + y1;

  out<<x0<<' '<<y0<<' '<<x1<<' '<<y1<<' '<<length1<<std::endl;
}

}
