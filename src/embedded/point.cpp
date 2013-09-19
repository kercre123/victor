#include "anki/embeddedCommon/point.h"
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
  } // namespace Embedded
} // namespace Anki