#include "anki/common/basestation/math/point_impl.h"

namespace Anki {
  
  void testPointInstantiation(void)
  {
    Point2f a2(1.f,2.f), b2(3.f,4.f);
    a2 += b2;
    
    Point3f a3(1.f,2.f,3.f), b3(4.f,5.f,6.f);
    a3 -= b3;
    
    return;
  }

} // namespace Anki