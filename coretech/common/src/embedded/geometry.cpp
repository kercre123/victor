#include "anki/embeddedCommon/geometry.h"
#include "anki/embeddedCommon/errorHandling.h"
#include "anki/embeddedCommon/utilities_c.h"

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

    template<> IN_DDR f32 Rectangle<f32>::get_width() const
    {
      return right - left;
    }

    template<> IN_DDR f64 Rectangle<f64>::get_width() const
    {
      return right - left;
    }

    template<> IN_DDR f32 Rectangle<f32>::get_height() const
    {
      return bottom - top;
    }

    template<> IN_DDR f64 Rectangle<f64>::get_height() const
    {
      return bottom - top;
    }

    template<> IN_DDR void Quadrilateral<f32>::Print() const
    {
      printf("{(%f,%f), (%f,%f), (%f,%f), (%f,%f)} ",
        this->corners[0].x, this->corners[0].y,
        this->corners[1].x, this->corners[1].y,
        this->corners[2].x, this->corners[2].y,
        this->corners[3].x, this->corners[3].y);
    }

    template<> IN_DDR void Quadrilateral<f64>::Print() const
    {
      printf("{(%f,%f), (%f,%f), (%f,%f), (%f,%f)} ",
        this->corners[0].x, this->corners[0].y,
        this->corners[1].x, this->corners[1].y,
        this->corners[2].x, this->corners[2].y,
        this->corners[3].x, this->corners[3].y);
    }
  } // namespace Embedded
} // namespace Anki