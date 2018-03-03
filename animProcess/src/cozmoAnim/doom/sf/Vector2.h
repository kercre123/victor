#pragma once

namespace sf
{
  
template <typename T>
class Vector2
{
public:
  Vector2(T a, T b) : x(a), y(b) {}
  Vector2() : Vector2( T{0}, T{0} ) {}
  
  template <class U>
  Vector2<T>& operator=( const Vector2<U>& other )
  {
    x = other.x;
    y = other.y;
    return *this;
  }
  template <class U>
  operator Vector2<U>() const
  {
    return Vector2<U>{static_cast<U>(x),static_cast<U>(y)};
  }

  
  T x;
  T y;
};
  
typedef Vector2<float> Vector2f;
typedef Vector2<int> Vector2i;

} // namespace

