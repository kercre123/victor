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
  } // namespace Embedded
} // namespace Anki