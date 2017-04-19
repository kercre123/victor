#include "anki/common/basestation/math/point_impl.h"

namespace Anki {

  char AxisToChar(AxisName axis)
  {
    switch( axis ) {
      case AxisName::X_NEG:
      case AxisName::X_POS:
        return 'X';
      case AxisName::Y_NEG:
      case AxisName::Y_POS:
        return 'Y';
      case AxisName::Z_NEG:
      case AxisName::Z_POS:
        return 'Z';
    }        
  }

  const char* AxisToCString(AxisName axis)
  {
    switch( axis ) {
      case AxisName::X_NEG: return "-X";
      case AxisName::X_POS: return "+X";
      case AxisName::Y_NEG: return "-Y";
      case AxisName::Y_POS: return "+Y";
      case AxisName::Z_NEG: return "-Z";
      case AxisName::Z_POS: return "+Z";
    }
  }

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
  
  Vec3f AxisToVec3f(AxisName axis)
  {
    switch( axis ) {
      case AxisName::X_NEG:
        return -X_AXIS_3D();
        
      case AxisName::X_POS:
        return  X_AXIS_3D();
        
      case AxisName::Y_NEG:
        return -Y_AXIS_3D();
        
      case AxisName::Y_POS:
        return  Y_AXIS_3D();
      
      case AxisName::Z_NEG:
        return -Z_AXIS_3D();
        
      case AxisName::Z_POS:
        return  Z_AXIS_3D();
    }
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
