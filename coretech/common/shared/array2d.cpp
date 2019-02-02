#include "coretech/common/shared/array2d_impl.h"

#include <iostream>

namespace Anki
{
  // Force there to be an instantiatino of the templated class with some
  // type, so we can see if it compiles
  void testArray2dInstantiation(void)
  {
    Array2d<float> stuff(100,100);

    // Exponentiate "stuff" in place:
    stuff.ApplyScalarFunction(expf);
  }
} // namespace Anki
