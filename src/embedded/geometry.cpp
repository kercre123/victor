#include "anki/embeddedCommon/geometry.h"
#include "anki/embeddedCommon/errorHandling.h"

namespace Anki
{
  namespace Embedded
  {
    template<> IN_DDR void Point<f32>::Print() const
    {
      printf("(%f,%f) ", this->x, this->y);
    }

    template<> IN_DDR void Point<f64>::Print() const
    {
      printf("(%f,%f) ", this->x, this->y);
    }

    template<> IN_DDR void Rectangle<f32>::Print() const
    {
      printf("(%f,%f)->(%f,%f) ", this->left, this->top, this->right, this->bottom);
    }

    template<> IN_DDR void Rectangle<f64>::Print() const
    {
      printf("(%f,%f)->(%f,%f) ", this->left, this->top, this->right, this->bottom);
    }

    template<> IN_DDR void Quadrilateral<f32>::Print() const
    {
      printf("{(%f,%f), (%f,%f), (%f,%f), (%f,%f)} ",
        this->points[0].x, this->points[0].y,
        this->points[1].x, this->points[1].y,
        this->points[2].x, this->points[2].y,
        this->points[3].x, this->points[3].y);
    }

    template<> IN_DDR void Quadrilateral<f64>::Print() const
    {
      printf("{(%f,%f), (%f,%f), (%f,%f), (%f,%f)} ",
        this->points[0].x, this->points[0].y,
        this->points[1].x, this->points[1].y,
        this->points[2].x, this->points[2].y,
        this->points[3].x, this->points[3].y);
    }
  } // namespace Embedded
} // namespace Anki