/**
 * File: memoryMapData_Boundary.h
 *
 * Author: Matt Michini
 * Date:   01/14/2018
 *
 * Description: Data for artificial play space boundaries that the robot should not cross
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "memoryMapData_Boundary.h"
#include "clad/types/memoryMap.h"
#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/rotatedRect.h"

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemoryMapData_Boundary::MemoryMapData_Boundary(const Point2f& from,
                                               const Point2f& to,
                                               const Pose3d& originPose,
                                               const float thickness_mm,
                                               RobotTimeStamp_t t)
: MemoryMapData(MemoryMapTypes::EContentType::Boundary, t, true)
, _boundarySegment(from, to)
, _thickness_mm(thickness_mm)
, _originPose(originPose)
{
  // Also compute a quad representing this boundary
  
  // Create a rotated rectangle from the underlying line segment, then convert to Quad2f
  // Add 90 degrees to the angle between to and from, in order to create the RotatedRect properly
  const float ang = M_PI_2_F + atan2f(to.y() - from.y(),
                                      to.x() - from.x());
  
  const float len = ComputeDistanceBetween(from, to);
  
  const float x0 = from.x() + 0.5 * _thickness_mm * cosf(ang);
  const float y0 = from.y() + 0.5 * _thickness_mm * sinf(ang);
  const float x1 = from.x() - 0.5 * _thickness_mm * cosf(ang);
  const float y1 = from.y() - 0.5 * _thickness_mm * sinf(ang);
  
  const auto rect = RotatedRectangle(x0, y0, x1, y1, len);
  _boundaryQuad = rect.GetQuad();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemoryMapTypes::MemoryMapDataPtr MemoryMapData_Boundary::Clone() const
{
  return MemoryMapDataWrapper<MemoryMapData_Boundary>(*this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMapData_Boundary::Equals(const MemoryMapData* other) const
{
  if ( other == nullptr || other->type != type ) {
    return false;
  }

  const MemoryMapData_Boundary* castPtr = static_cast<const MemoryMapData_Boundary*>( other );

  const bool isSameOrigin = _originPose == castPtr->_originPose;
  const bool isSameFrom = IsNearlyEqual( _boundarySegment.GetFrom(), castPtr->_boundarySegment.GetFrom() );
  const bool isSameTo = IsNearlyEqual( _boundarySegment.GetTo(), castPtr->_boundarySegment.GetTo() );

  return isSameOrigin && isSameFrom && isSameTo;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ExternalInterface::ENodeContentTypeEnum MemoryMapData_Boundary::GetExternalContentType() const
{
  return ExternalInterface::ENodeContentTypeEnum::Boundary;
}

  
} // namespace Vector
} // namespace Anki
