// This file exists simply to avoid confusion with including _impl files:
// Image, which doesn't really need an _impl file, inherits from Array2d, which
// does use an _impl file (since it's templated).  But it's weird for people
// who want to use Image to need to know that they need to include array2d_impl.

#include "coretech/vision/engine/image.h"
#include "coretech/common/engine/array2d_impl.h"
