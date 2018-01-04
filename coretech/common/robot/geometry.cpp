/**
File: geometry.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/common/robot/geometry.h"
#include "coretech/common/robot/errorHandling.h"
#include "coretech/common/robot/utilities_c.h"

namespace Anki
{
  namespace Embedded
  {
    template<> void Point<f32>::Print() const
    {
      CoreTechPrint("(%f,%f) ", this->x, this->y);
    }

    template<> void Point<f64>::Print() const
    {
      CoreTechPrint("(%f,%f) ", this->x, this->y);
    }

    template<> void Point3<f32>::Print() const
    {
      CoreTechPrint("(%f,%f,%f) ", this->x, this->y, this->z);
    }

    template<> void Point3<f64>::Print() const
    {
      CoreTechPrint("(%f,%f,%f) ", this->x, this->y, this->z);
    }

    template<> void Rectangle<f32>::Print() const
    {
      CoreTechPrint("(%f,%f)->(%f,%f) ", this->left, this->top, this->right, this->bottom);
    }

    template<> void Rectangle<f64>::Print() const
    {
      CoreTechPrint("(%f,%f)->(%f,%f) ", this->left, this->top, this->right, this->bottom);
    }

    template<> void Quadrilateral<f32>::Print() const
    {
      CoreTechPrint("{(%f,%f), (%f,%f), (%f,%f), (%f,%f)} ",
        this->corners[0].x, this->corners[0].y,
        this->corners[1].x, this->corners[1].y,
        this->corners[2].x, this->corners[2].y,
        this->corners[3].x, this->corners[3].y);
    }

    template<> void Quadrilateral<f64>::Print() const
    {
      CoreTechPrint("{(%f,%f), (%f,%f), (%f,%f), (%f,%f)} ",
        this->corners[0].x, this->corners[0].y,
        this->corners[1].x, this->corners[1].y,
        this->corners[2].x, this->corners[2].y,
        this->corners[3].x, this->corners[3].y);
    }
  } // namespace Embedded
} // namespace Anki
