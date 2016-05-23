/**
 * File: mathStructures
 *
 * Author: raul
 * Created: 04/15/15
 *
 * Description: Abstracting math structures from their implementation in case we need to change libraries.
 *
 * Copyright: Anki, Inc.
 *
 **/

#ifndef __Util_MathStructures_H__
#define __Util_MathStructures_H__

#include "kazmath/src/kazmath.h"
#include <float.h>

namespace Anki {
namespace Util {
namespace Math {

typedef kmScalar Scalar;
typedef ::kmVec2 Vector2;
typedef ::kmVec3 Vector3;
typedef ::kmMat3 Matrix3;
typedef ::kmMat4 Matrix4;
typedef ::kmQuaternion Quaternion;
typedef ::kmPlane Plane;
typedef ::kmAABB2 AABB2;
typedef ::kmAABB3 AABB3;
typedef ::kmRay2 Ray2;
typedef ::kmRay3 Ray3;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Additional functionality
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

// expand aabb2 so that given point is contained
void AABB2ExpandToContain(AABB2& aabb2, const Vector2& point);

// return true if the given aabbs collide, false otherwise
// CollideFull   : if the edges are touching (have same value), it returns false
// CollideOrTouch: if the edges are touching (have same value), it returns true
bool AABB2CollideFull(const AABB2& one, const AABB2& other, const float epsilon=FLT_EPSILON);
bool AABB2CollideOrTouch(const AABB2& one, const AABB2& other, const float epsilon=FLT_EPSILON);

} // namespace
} // namespace
} // namespace

#endif
