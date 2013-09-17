#include "anki/embeddedCommon.h"

namespace Anki
{
  namespace Embedded
  {
    template<> void Point<f32>::Print() const
    {
      printf("(%f,%f) ", this->x, this->y);
    }

    template<> void Point<f64>::Print() const
    {
      printf("(%f,%f) ", this->x, this->y);
    }
  } // namespace Embedded
} // namespace Anki