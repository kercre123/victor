/**
 * File: poseBase.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 6/19/2014
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description: Defines a base class for Pose2d and Pose3d to inherit from in
 *                order to share pose tree capabilities.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef _ANKICORETECH_MATH_POSEBASE_H_
#define _ANKICORETECH_MATH_POSEBASE_H_

#include "anki/common/basestation/math/matrix.h"
#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/math/quad.h"
#include "anki/common/basestation/math/rotation.h"
#include "anki/common/shared/radians.h"

#include "anki/common/basestation/exceptions.h"

#include <list>

namespace Anki {

  // Forward declarations of types used below:
  //  typedef Point3f Vec3f;
  //template<typename T> class Matrix;
  

  // TODO: Have Pose2d and Pose3d inherit from an (abstract?) base class?
  //  Then the base class could define the common elements like the parent
  //  pointer, the GetTreeDepth() method, and the GetWithRespectTo method.
  template <class PoseNd>
  class PoseBase
  {
  public:
 
    PoseBase();
    PoseBase(const PoseNd* parentPose);

    void SetParent(const PoseNd* otherPose) { _parent = otherPose; }
    const PoseNd* GetParent() const { return _parent; }
    
    bool IsOrigin() const { return _parent == nullptr; }
    
    const PoseNd& FindOrigin(const PoseNd& forPose) const;
    
    // Origins
    static PoseNd* World;
    static std::list<PoseNd> Origins;
    static PoseNd& AddOrigin();
    static PoseNd& AddOrigin(const PoseNd& origin);

  protected:
    
    const PoseNd* _parent;
    unsigned int GetTreeDepth(const PoseNd* poseNd) const;
    
    bool GetWithRespectTo(const PoseNd& from, const PoseNd& to,
                          PoseNd& newPose) const;

  };
  
  
} // namespace Anki

#endif // _ANKICORETECH_MATH_POSEBASE_H_