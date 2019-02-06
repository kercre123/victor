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

#ifndef __COMMON_ENGINE_MATH_BRESENHAM_LINE_2D_H__
#define __COMMON_ENGINE_MATH_BRESENHAM_LINE_2D_H__

#include "coretech/common/shared/math/point_fwd.h"
#include <vector>

namespace Anki {

std::vector<Point2i> GetBresenhamLine(const Point2i& p, const Point2i& q, bool fourConnectedLine = false);

class BresenhamLinePixelIterator
{
  public:
    BresenhamLinePixelIterator(const Point2i& p, const Point2i& q);

    void Next();

    bool Done() const;

    const Point2i& Get() const;

  private:
    Point2i _curr;

    // tracking error for the current pixel point
    s32 _error;
    
    // stride to traverse in x and y axis
    u32 _dx;
    u32 _dy;
    
    // sign of the increment when following the line
    s32 _xInc;
    s32 _yInc;

    // how many pixels remain to generate
    // is -1 when none are left
    s32 _counter;
};

}

#endif