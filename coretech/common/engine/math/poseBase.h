/**
 * File: poseBase.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 6/19/2014
 *
 *
 * Description: Defines a base class for Pose2d and Pose3d to inherit from in
 *                order to share pose tree capabilities.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __Anki_Common_Math_PoseBase_H__
#define __Anki_Common_Math_PoseBase_H__

#include "coretech/common/shared/math/matrix.h"
#include "coretech/common/shared/math/point_fwd.h"
#include "coretech/common/engine/math/quad.h"
#include "coretech/common/shared/math/rotation.h"
#include "coretech/common/shared/math/radians.h"

#include "coretech/common/engine/exceptions.h"

#include <string>

namespace Anki {

  using PoseID_t = uint32_t;
  
  // Pose2d and Pose3d inherit from this base class, which defines the common
  // elements like the parent pointer, the GetTreeDepth() method, and the
  // GetWithRespectTo method.
  template <class PoseNd, class TransformNd>
  class PoseBase
  {
  public:
  
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Methods
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    bool IsNull() const { return (nullptr == _node); }
    
    // init/destruction, copy, assignment
    virtual ~PoseBase();

    //
    // Simple accessors:
    //
    void SetName(const std::string& newName);
    const std::string& GetName() const;
    
    // Get the ID assigned to this pose. IDs default to 0.
    PoseID_t GetID() const;
    
    // Set the ID for this pose. NOTE: ID is *not* copied with pose!
    void SetID(PoseID_t newID);
    
    TransformNd const& GetTransform() const&;
    TransformNd &      GetTransform() &;
    TransformNd        GetTransform() &&;
    
    // Set / check parent relationships:
    void ClearParent();
    void SetParent(const PoseNd& otherPose);

    // "Roots" are the top of a pose tree and thus have no parent
    bool     IsRoot()    const;
    bool     HasParent() const;
    PoseID_t GetRootID() const; // NOTE: more efficient than FindRoot().GetID()
    
    // Note that HasSameRootAs will be true if this is other's root or vice versa (a root's root is itself)
    bool     HasSameRootAs(const PoseNd& otherPose)   const;
    bool     HasSameParentAs(const PoseNd& otherPose) const;
    
    // Get a string suitable for log messages which describes this node's parent
    std::string GetParentString() const;
    
    // Check if given pose is this pose's parent (or vice versa)
    // Cheaper than calling this->GetParent and then comparing it to pose
    bool IsChildOf(const PoseNd& otherPose) const;
    bool IsParentOf(const PoseNd& otherPose) const;
    
    // Composition with another PoseNd
    void   PreComposeWith(const PoseNd& other)  { GetTransform().PreComposeWith(other.GetTransform()); }
    void   operator*=(const PoseNd& other)      { GetTransform() *= other.GetTransform();              }
    PoseNd operator*(const PoseNd& other) const;
    
    // Inversion
    void   Invert(void);
    PoseNd GetInverse(void) const;
  
    // Creates a new PoseNd around the underlying root/parent node
    // This is "cheap", but not "free". Consider using one of the above helpers for comparing relationships b/w poses.
    // NOTE: These return by value, not reference. Do not store references to members in line! I.e., never do:
    //   const Foo& = somePose.GetWithRespectToRoot().GetFoo(); // Foo will immediately be invalidated
    // Either store "somePoseWrtRoot" and then get Foo& from it, or make Foo a copy.
    PoseNd FindRoot()  const;
    PoseNd GetParent() const;
    
    // Check for exact numerical equality of Transform and matching parents/names. Generally only useful for unit tests.
    bool operator==(const PoseNd &other) const { return ((GetTransform() == other.GetTransform()) &&
                                                         HasSameParentAs(other) &&
                                                         GetName()==other.GetName()); }
    
    // Useful for debugging and logging messages: print/return a string indicating the path up to the root node
    std::string GetNamedPathToRoot(bool showTranslations) const;
    void        PrintNamedPathToRoot(bool showTranslations) const;
    
    // An "unowned" parent can result when, for example, Pose A uses a temporary Pose B as its parent and then
    // Pose B goes out of scope. No Pose actually "owns" the node inside Pose B, but Pose A still refers to it.
    // If unowned parents are not allowed, then a parent will be ANKI_VERIFY'd to have an owner
    // each time its node's GetParent() method is called. Normally, unowned parents are *not* allowed
    // but this can be globally adjusted if desired (e.g. for unit tests).
    static bool AreUnownedParentsAllowed();
    static void AllowUnownedParents(bool tf);
    bool IsOwned() const;
    uint32_t GetNodeOwnerCount() const; // mostly useful for unit tests
    
  protected:
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Methods
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
    // We don't want to actually publicly create PoseBase objects: just derived
    // classes like Pose2d and Pose3d
    PoseBase();
    PoseBase(const TransformNd& transform, const PoseNd& parentPose, const std::string& name = "");
    PoseBase(const TransformNd& transform, const std::string& name = "");
    PoseBase(const PoseNd& parentPose, const std::string& name);
    
    // Copy/assignment. NOTE: IDs are *NOT* copied and name is appended with "_COPY".
    PoseBase(const PoseBase& other);
    PoseBase& operator=(const PoseBase& other);
    
    // COZMO-10891: Removing RValue construction/assignment for trying to debug this ticket
    // // Rvalue construction/assignment. NOTE: IDS *are* moved and name is preserved.
    PoseBase(PoseBase&& other) = delete;
    PoseBase& operator=(PoseBase&& other) = delete;

    static bool GetWithRespectTo(const PoseNd& from, const PoseNd& to, PoseNd& newPose);
    
    // Create a new Pose object which "wraps" an existing node
    class PoseTreeNode;
    void WrapExistingNode(const std::shared_ptr<PoseTreeNode>& node) { _node = node; _node->AddOwner(); }
        
  private:
    
    std::shared_ptr<PoseTreeNode> _node;
    
    static bool _areUnownedParentsAllowed;
  };
  
} // namespace Anki

#endif /* __Anki_Common_Math_PoseBase_H__ */
