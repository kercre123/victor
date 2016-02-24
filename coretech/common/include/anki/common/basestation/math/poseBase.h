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
#include <set>
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
  
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Methods
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
    // init/destruction, copy, assignment
    virtual ~PoseBase();

    // accessors
    void SetParent(const PoseNd* otherPose);
    const PoseNd* GetParent() const { Dev_AssertIsValidParentPointer(_parentPtr, this); return _parentPtr; }

    void SetName(const std::string& newName) { _name = newName; }
    const std::string& GetName() const { return _name; }
    
    // origin
    bool IsOrigin() const { Dev_AssertIsValidParentPointer(_parentPtr, this); return _parentPtr == nullptr; }
    const PoseNd& FindOrigin(const PoseNd& forPose) const;
    
  protected:
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Methods
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
    // We don't want to actually publicly create PoseBase objects: just derived
    // classes like Pose2d and Pose3d
    PoseBase();
    PoseBase(const PoseNd* parentPose, const std::string& name);
    PoseBase(const PoseBase& other);
    PoseBase& operator=(const PoseBase& other);
    PoseBase(const PoseBase&& other) = delete;
   
    unsigned int GetTreeDepth(const PoseNd* poseNd) const;
    
    bool GetWithRespectTo(const PoseNd& from, const PoseNd& to,
                          PoseNd& newPose) const;
    
    static std::string GetNamedPathToOrigin(const PoseNd& startPose, bool showTranslations);
    static void        PrintNamedPathToOrigin(const PoseNd& startPose, bool showTranslations);
    
  private:

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Dev methods
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    // updates children bookkeping variables for old and new parents
    static void Dev_SwitchParent(const PoseNd* oldParent, const PoseNd* newParent, const PoseBase<PoseNd>* childBasePointer);
    // asserts that the parent is valid and that the child is a current valid child of the parent
    static void Dev_AssertIsValidParentPointer(const PoseNd* parent, const PoseBase<PoseNd>* childBasePointer);

    // note I'm not alive anymore, I can't be asked about parents
    static void Dev_PoseDestroyed(const PoseBase<PoseNd>* basePointer);
    // note I'm alive, and people can ask me about my children
    static void Dev_PoseCreated(const PoseBase<PoseNd>* basePointer);

    // static set of valid poses
    using PoseCPtrSet = std::set<const PoseNd*>;
    static PoseCPtrSet& Dev_GetValidPoses() {
      static PoseCPtrSet sDev_ValidPoses;
      return sDev_ValidPoses;
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Attributes
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    // _parentPtr is private because in order to make sure parent pointers are not used when the
    // parent is destroyed, we track reference counts when parents are switched. Do not use _parentPtr directly,
    //use GetParent/SetParent that check pointer validity.
    // Note: This should probably be an std::weak_ptr or std::shared_ptr, but our API is well defined at the moment,
    // and switching to smart pointers seems daunting.
    const PoseNd* _parentPtr;

    // name
    std::string _name;

    // bookkeeping variables to ensure parent pointer validity. They should be empty in release.
    // During debug, however, parents keep track of their children so that we can validate proper parenthood
    mutable std::set<const PoseNd*> _devChildrenPtrs; // my current valid children
  };
  
} // namespace Anki

#endif // _ANKICORETECH_MATH_POSEBASE_H_