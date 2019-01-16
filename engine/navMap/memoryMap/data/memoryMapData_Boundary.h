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

#ifndef ANKI_COZMO_MEMORY_MAP_DATA_Boundary_H
#define ANKI_COZMO_MEMORY_MAP_DATA_Boundary_H

#include "memoryMapData.h"

#include "coretech/common/engine/math/lineSegment2d.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/math/quad.h"
#include "coretech/common/engine/robotTimeStamp.h"

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NavMemoryMapQuadData_Boundary
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct MemoryMapData_Boundary : public MemoryMapData
{
  // constructor
  MemoryMapData_Boundary(const Point2f& from,      // beginning of line segment describing boundary
                         const Point2f& to,        // end of line segment describing boundary
                         const Pose3d& originPose, // the previous points are w.r.t this pose
                         const float thickness_mm, // thickness of the boundary
                         RobotTimeStamp_t t);
  
  // create a copy of self (of appropriate subclass) and return it
  MemoryMapDataPtr Clone() const override;
  
  // compare to INavMemoryMapQuadData and return bool if the data stored is the same
  bool Equals(const MemoryMapData* other) const override;
  
  virtual ExternalInterface::ENodeContentTypeEnum GetExternalContentType() const override;
  
  // Get the quad representing the boundary
  Quad2f GetQuad();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // If you add attributes, make sure you add them to ::Equals and ::Clone (if required)

  // The underlying line segment that makes up the boundary
  const LineSegment _boundarySegment;
  
  // Thickness of the boundary
  const float _thickness_mm;
  
  // Base pose which the line segment is with respect to
  const Pose3d _originPose;

  // _boundaryQuad is a quad representing the boundary. This duplicates data (blergh)
  Quad2f _boundaryQuad;
  
  static bool HandlesType(EContentType otherType) {
    return otherType == EContentType::Boundary;
  }
};
 
} // namespace
} // namespace

#endif //
