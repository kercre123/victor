/**
 * File: bresenhamLine2d.h
 *
 * Author: Arjun Menon
 * Created: 2018-11-01
 *
 * Description: 
 * Defines a simple implementation of 2-dimensional bresenham line
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "coretech/common/engine/math/bresenhamLine2d.h"
#include "coretech/common/shared/math/point.h"

namespace Anki {

std::vector<Point2i> GetBresenhamLine(const Point2i& p, const Point2i& q, bool fourConnectedLine)
{
  s32 x1 = p.x();
  s32 y1 = p.y();
  s32 x2 = q.x();
  s32 y2 = q.y();
  
  std::vector<Point2i> points;

  if(fourConnectedLine) {
    BresenhamLinePixelIterator iter(p, q);
    while(!iter.Done()) {
      points.push_back(iter.Get());
      iter.Next();
    }
  } else {
    bool steep = false; // whether the stride of dy is greater than dx
    if(std::abs((s64)x2-x1) < std::abs((s64)y2-y1)) {
      // reflect the line so we can reuse the algorithm
      // this allows us to continue advancing x-coordinate by 1
      // while we accumulate the error term in the y-coordinate
      std::swap(x1,y1);
      std::swap(x2,y2);
      // after we generate the points on the line, we'll reflect again
      // in order to recover the true line
      steep = true;
    }

    // enforce the line from P1 -> P2 by going in the positive-X direction
    //  i.e. goes from min(x1,x2) to max(x1,x2)
    if(x1 > x2) {
      std::swap(x1, x2);
      std::swap(y1, y2);
    }
    const int yInc = y2 > y1 ? 1 : -1;
    
    const float derror = std::abs(float(x2-x1) / float(y2-y1));

    s32 currX = x1;
    s32 currY = y1;

    float errorInY = 0.f;
    points.reserve(x2 - x1 + 1);
    for(; currX <= x2; currX++) {
      if(!steep) {
        points.emplace_back(currX, currY);
      } else {
        // reflect in the output return value
        points.emplace_back(currY, currX);
      }
      // compute the next point along the line
      // - advance the x-coordinate by 1
      // - determine the residual error in shifting y along the line
      errorInY += derror;
      if(errorInY >= 0.5f) {
        currY += yInc;
        // note: if the line is parallel to the Y-axis,
        // then errorInY will never grow past 0.5
        errorInY -= 1.f;
      }
    }
  }

  return points;
}

BresenhamLinePixelIterator::BresenhamLinePixelIterator(const Point2i& p, const Point2i& q)
{
  _curr = p;
  
  if(p.x() > q.x()) {
    _dx = p.x() - q.x();
    _xInc = -1;
  } else {
    _dx = q.x() - p.x();
    _xInc = 1;
  }

  if(p.y() > q.y()) {
    _dy = p.y() - q.y();
    _yInc = -1;
  } else {
    _dy = q.y() - p.y();
    _yInc = 1;
  }

  _error = 0;
  _counter = _dx+_dy;
}

void BresenhamLinePixelIterator::Next()
{
  // note: signs of accumulating error from dx or dy are reversed to ensure
  // that when we take a step in a direction, the opposite step will reduce
  // the tracking error for following the true line
  const int errorX = _error + _dy;
  const int errorY = _error - _dx;
  if(std::abs(errorX) < std::abs(errorY)) {
    _curr.x() += _xInc;
    _error = errorX;
  } else {
    _curr.y() += _yInc;
    _error = errorY;
  }
  --_counter;
}

bool BresenhamLinePixelIterator::Done() const
{
  return _counter < 0;
}

const Point2i& BresenhamLinePixelIterator::Get() const
{
  return _curr;
}

}