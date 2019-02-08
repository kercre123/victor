#include "coretech/common/shared/math/point.h"

namespace Anki {

// comparison operation tests
namespace
{
  constexpr const Point3f A(1.f, 2.f, 3.f);
  constexpr const Point3f B(4.f, 5.f, 6.f);
  constexpr const Point3f C(0.5f, 1.5f, 2.f);
  constexpr const Point3f D(1.f, 3.f, 4.f);
  constexpr const Point3f E(0.5f, 2.f, 2.f);
  constexpr const Point3f F(2.f, 3.f, 2.f);
  
  // All A strictly less than B
  static_assert(A.AllLT(B), "");
  static_assert(A.AllLTE(B), "");
  static_assert(A.AnyLT(B), "");
  static_assert(A.AnyLTE(B), "");
  static_assert(!A.AllGT(B), "");
  static_assert(!A.AllGTE(B), "");
  static_assert(!A.AnyGT(B), "");
  static_assert(!A.AnyGTE(B), "");
  
  // All A strictly greater than C
  static_assert(A.AllGT(C), "");
  static_assert(A.AllGTE(C), "");
  static_assert(!A.AllLT(C), "");
  static_assert(!A.AllLTE(C), "");
  static_assert(!A.AnyLT(C), "");
  static_assert(!A.AnyLTE(C), "");
  
  // All A <= D
  static_assert(A.AllLTE(D), "");
  static_assert(A.AnyLTE(D), "");
  static_assert(A.AnyLT(D), "");
  static_assert(!A.AllLT(D), ""); // A.x() == D.x()
  
  // All A >= E
  static_assert(A.AllGTE(E), "");
  static_assert(A.AnyGTE(E), "");
  static_assert(A.AnyGT(E), "");
  static_assert(!A.AllGT(E), ""); // A.y() == E.y()
  
  // Mixed
  static_assert(A.AnyGT(F), "");
  static_assert(A.AnyLT(F), "");
  static_assert(A.AnyGTE(F), "");
  static_assert(A.AnyLTE(F), "");
  static_assert(!A.AllGT(F), "");
  static_assert(!A.AllLT(F), "");
  static_assert(!A.AllGTE(F), "");
  static_assert(!A.AllLTE(F), "");

  // Join and Slice
  constexpr const auto AB = Join(A,B);
  static_assert(AB == Point<6,float>{1.f, 2.f, 3.f, 4.f, 5.f, 6.f}, "");
  static_assert(AB.Slice<1,3>() == Point3f{2.f, 3.f, 4.f}, "");

  // TODO: operator checks
}

// DotProductAndLength
namespace {
  constexpr Point3f x(1.f, 2.f, 3.f);
  constexpr Point3f y(4.f, 5.f, 6.f);
  
  constexpr const float eps = 10.f*std::numeric_limits<float>::epsilon();
  
  static_assert(DotProduct(x,y) == 32.f, "");
  static_assert(Util::IsNear(DotProduct(x,x), x.LengthSq(), eps), "");
  
  constexpr Point<5,float> a(1.f, 2.f, 3.f, 4.f, 5.f);
  constexpr Point<5,float> b(2.f, 3.f, 4.f, 5.f, 6.f);
  
  static_assert(DotProduct(a,b) == 70.f, "");
  static_assert(Util::IsNear(DotProduct(a,a), a.LengthSq(), eps), "");
}

constexpr char AxisToChar(AxisName axis)
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

constexpr Vec3f AxisToVec3f(AxisName axis)
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
