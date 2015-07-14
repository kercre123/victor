#include "anki/common/basestation/math/point_impl.h"

namespace Anki {
  
  const Vec2f& X_AXIS_2D() {
    static const Vec2f xAxis(1.f, 0.f);
    return xAxis;
  }
  
  const Vec2f& Y_AXIS_2D() {
    static const Vec2f yAxis(0.f, 1.f);
    return yAxis;
  }
  
  const Vec3f& X_AXIS_3D() {
    static const Vec3f xAxis(1.f, 0.f, 0.f);
    return xAxis;
  }
  
  const Vec3f& Y_AXIS_3D() {
    static const Vec3f yAxis(0.f, 1.f, 0.f);
    return yAxis;
  }
  
  const Vec3f& Z_AXIS_3D() {
    static const Vec3f zAxis(0.f, 0.f, 1.f);
    return zAxis;
  }
  
  
  void testPointInstantiation(void)
  {
    Point2f a2(1.f,2.f), b2(3.f,4.f);
    a2 += b2;
    
    Point3f a3(1.f,2.f,3.f), b3(4.f,5.f,6.f);
    a3 -= b3;
    
    return;
  }

} // namespace Anki