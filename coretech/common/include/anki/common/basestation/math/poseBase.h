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
#include <string>

namespace Anki {

  // Forward declarations of types used below:
  //  typedef Point3f Vec3f;
  //template<typename T> class Matrix;
  

  // Pose2d and Pose3d inherit from this base class, which defines the common
  // elements like the parent pointer, the GetTreeDepth() method, and the
  // GetWithRespectTo method.
  template <class PoseNd>
  class PoseBase
  {
  public:
 
    void SetParent(const PoseNd* otherPose) { CORETECH_ASSERT(otherPose != this); _parent = otherPose; }
    const PoseNd* GetParent() const { return _parent; }
    
    bool IsOrigin() const { return _parent == nullptr; }
    
    const PoseNd& FindOrigin(const PoseNd& forPose) const;
    
    // Origins
    //static const PoseNd* GetWorldOrigin() { return _sWorld; }
    //static PoseNd& AddOrigin();
    //static PoseNd& AddOrigin(const PoseNd& origin);

    void SetName(const std::string& newName) { _name = newName; }
    const std::string& GetName() const { return _name; }
    
  protected:
    
    // We don't want to actually publicly create PoseBase objects: just derived
    // classes like Pose2d and Pose3d
    PoseBase();
    PoseBase(const PoseNd* parentPose, const std::string& name);
    
    //static const PoseNd* _sWorld;
    //static std::list<PoseNd> Origins;
    
    const PoseNd* _parent;
    std::string   _name;
    
    unsigned int GetTreeDepth(const PoseNd* poseNd) const;
    
    bool GetWithRespectTo(const PoseNd& from, const PoseNd& to,
                          PoseNd& newPose) const;
    
    static std::string GetNamedPathToOrigin(const PoseNd& startPose, bool showTranslations);
    static void        PrintNamedPathToOrigin(const PoseNd& startPose, bool showTranslations);

  };
  
  
} // namespace Anki


// TODO: Ideally, we'd only need to include poseBase_impl.h from cpp files...
#include "anki/common/basestation/math/poseBase_impl.h"

#endif // _ANKICORETECH_MATH_POSEBASE_H_