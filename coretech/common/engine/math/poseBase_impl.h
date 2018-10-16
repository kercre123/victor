/**
 * File: poseBase.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 6/19/2014
 *
 * Description: Implements a base class for Pose2d and Pose3d to inherit from in
 *                order to share pose tree capabilities.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef _ANKICORETECH_MATH_POSEBASE_IMPL_H_
#define _ANKICORETECH_MATH_POSEBASE_IMPL_H_

#include "coretech/common/engine/math/matrix.h"
#include "coretech/common/engine/math/point.h"
#include "coretech/common/engine/math/quad.h"
#include "coretech/common/engine/math/rotation.h"
#include "coretech/common/shared/radians.h"

#include "coretech/common/engine/math/poseBase.h"
#include "coretech/common/engine/math/poseTreeNode.h"

#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"


namespace Anki {
  
  template<class PoseNd, class TransformNd>
  bool PoseBase<PoseNd,TransformNd>::_areUnownedParentsAllowed = false;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  PoseBase<PoseNd,TransformNd>::PoseBase()
  {
    // NOTE: does not create a PoseTreeNode at all: this does not "wrap" anything!
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  PoseBase<PoseNd,TransformNd>::PoseBase(const TransformNd& transform, const PoseNd& parentPose, const std::string& name)
  : _node(new PoseTreeNode(transform, parentPose._node, name))
  {
    _node->AddOwner();
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  PoseBase<PoseNd,TransformNd>::PoseBase(const TransformNd& transform, const std::string& name)
  : _node(new PoseTreeNode(transform, nullptr, name))
  {
    _node->AddOwner();
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  PoseBase<PoseNd,TransformNd>::PoseBase(const PoseNd& parentPose, const std::string& name)
  : _node(new PoseTreeNode(TransformNd(), parentPose._node, name))
  {
    _node->AddOwner();
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // destructor
  template<class PoseNd, class TransformNd>
  PoseBase<PoseNd,TransformNd>::~PoseBase()
  {
    if(!IsNull())
    {
      _node->RemoveOwner();
      if(!_node->IsOwned() && !AreUnownedParentsAllowed())
      {
        // Trying to diagnose COZMO-10891: if this was the last owner, the node should have use_count==1
        ANKI_VERIFY(_node.use_count() == 1,
                    "PoseBase.Destructor.NotLastOwner",
                    "Removing pose %d(%s)'s node's last known owner, but it still has use_count=%u",
                    GetID(), GetName().c_str(), (uint32_t)_node.use_count());
      }
    }
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // copy constructor
  template<class PoseNd, class TransformNd>
  PoseBase<PoseNd,TransformNd>::PoseBase(const PoseBase& other)
  : _node(new PoseTreeNode(*other._node)) // don't share Nodes with other! copy the contents!
  {
    _node->AddOwner();
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  PoseBase<PoseNd,TransformNd>& PoseBase<PoseNd,TransformNd>::operator=(const PoseBase& other)
  {
    if(this != &other)
    {
      if(other.IsNull())
      {
        _node->RemoveOwner();
        if(!_node->IsOwned() && !AreUnownedParentsAllowed())
        {
          // Trying to diagnose COZMO-10891: if this was the last owner, the node should have use_count==1
          ANKI_VERIFY(_node.use_count() == 1,
                      "PoseBase.AssignmentOperator.NotLastOwner",
                      "Removing pose %d(%s)'s node's last known owner, but it still has use_count=%u",
                      GetID(), GetName().c_str(), (uint32_t)_node.use_count());
        }
        _node.reset();
      }
      else
      {
        // don't share Nodes with other! assign the contents! note that there's no new owner of _node here!
        *_node = *other._node;
      }
    }
    return *this;
  }
  
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  // rvalue constructor
//  template<class PoseNd, class TransformNd>
//  PoseBase<PoseNd,TransformNd>::PoseBase(PoseBase&& other)
//  : _node(std::move(other._node))
//  {
//    
//  }
//  
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  // rvalue assignment
//  template<class PoseNd, class TransformNd>
//  PoseBase<PoseNd,TransformNd>& PoseBase<PoseNd,TransformNd>::operator=(PoseBase&& other)
//  {
//    if(this != &other)
//    {
//      std::swap(_node, other._node);
//    }
//    return *this;
//  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  inline void PoseBase<PoseNd,TransformNd>::SetName(const std::string& newName)
  {
    DEV_ASSERT(!IsNull(), "PoseBase.SetName.NullNode");
    _node->SetName(newName);
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  inline const std::string& PoseBase<PoseNd,TransformNd>::GetName() const
  {
    DEV_ASSERT(!IsNull(), "PoseBase.GetName.NullNode");
    return _node->GetName();
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  inline PoseID_t PoseBase<PoseNd,TransformNd>::GetID() const
  {
    DEV_ASSERT(!IsNull(), "PoseBase.GetID.NullNode");
    return _node->GetID();
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  inline void PoseBase<PoseNd,TransformNd>::SetID(PoseID_t newID)
  {
    DEV_ASSERT(!IsNull(), "PoseBase.SetID.NullNode");
    _node->SetID(newID);
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  inline TransformNd const& PoseBase<PoseNd,TransformNd>::GetTransform() const&
  {
    DEV_ASSERT(!IsNull(), "PoseBase.GetTransformConst.NullNode");
    return _node->GetTransform();
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  inline TransformNd& PoseBase<PoseNd,TransformNd>::GetTransform() &
  {
    DEV_ASSERT(!IsNull(), "PoseBase.GetTransform.NullNode");
    return _node->GetTransform();
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  inline TransformNd PoseBase<PoseNd,TransformNd>::GetTransform() &&
  {
    DEV_ASSERT(!IsNull(), "PoseBase.GetTransformRvalue.NullNode");
    return _node->GetTransform();
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  inline bool PoseBase<PoseNd,TransformNd>::IsRoot()    const
  {
    return (IsNull() ? true : _node->IsRoot());
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  inline bool PoseBase<PoseNd,TransformNd>::HasParent() const
  {
    return (IsNull() ? false : _node->HasParent());
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  inline PoseID_t PoseBase<PoseNd,TransformNd>::GetRootID() const
  {
    return (IsNull() ? 0 : _node->GetRootID());
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  inline bool PoseBase<PoseNd,TransformNd>::IsChildOf(const PoseNd& otherPose) const
  {
    return (IsNull() || otherPose.IsNull() ? false : _node->IsChildOf(*otherPose._node));
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  inline bool PoseBase<PoseNd,TransformNd>::IsParentOf(const PoseNd& otherPose) const
  {
    return (IsNull() || otherPose.IsNull() ? false : _node->IsParentOf(*otherPose._node));
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  inline void PoseBase<PoseNd,TransformNd>::Invert(void)
  {
    GetTransform().Invert();
    ClearParent();
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  inline PoseNd PoseBase<PoseNd,TransformNd>::GetInverse(void) const
  {
    return GetTransform().GetInverse();
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  void PoseBase<PoseNd,TransformNd>::SetParent(const PoseNd& otherPose)
  {
    DEV_ASSERT(!IsNull(), "PoseBase.SetParent.NullNode");
    DEV_ASSERT(&otherPose != this, "PoseBase.SetParent.ParentCannotBeSelf");
    if(!AreUnownedParentsAllowed())
    {
      ANKI_VERIFY(otherPose.IsOwned(),
                  "PoseBase.SetParent.UnownedParent",
                  "Setting parent of %d(%s) to unowned pose %d(%s)",
                  GetID(), GetName().c_str(), otherPose.GetID(), otherPose.GetName().c_str());
    }
    _node->SetParent(otherPose._node);
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  void PoseBase<PoseNd,TransformNd>::ClearParent()
  {
    DEV_ASSERT(!IsNull(), "PoseBase.ClearParent.NullNode");
    _node->SetParent(nullptr);
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  void PoseBase<PoseNd,TransformNd>::PrintNamedPathToRoot(bool showTranslations) const
  {
    std::string str = GetNamedPathToRoot(showTranslations);
    fprintf(stdout, "Path to root: %s\n", str.c_str());
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  std::string PoseBase<PoseNd,TransformNd>::GetNamedPathToRoot(bool showTranslations) const
  {
    if(IsNull())
    {
      return "NullNode";
    }
    return _node->GetNamedPathToRoot(showTranslations);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  bool PoseBase<PoseNd,TransformNd>::HasSameParentAs(const PoseNd& otherPose) const
  {
    if(IsNull() || otherPose.IsNull())
    {
      return false;
    }
    
    return _node->HasSameParentAs(*otherPose._node);
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  PoseNd PoseBase<PoseNd,TransformNd>::operator*(const PoseNd& other) const
  {
    PoseNd newPose;
    newPose.GetTransform() = (GetTransform() * other.GetTransform());
    return newPose;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  bool PoseBase<PoseNd,TransformNd>::HasSameRootAs(const PoseNd& otherPose) const
  {
    if(IsNull() || otherPose.IsNull())
    {
      return false;
    }
    
    return _node->HasSameRootAs(*otherPose._node);
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  std::string PoseBase<PoseNd,TransformNd>::GetParentString() const
  {
    std::stringstream ss;
    ss << "Parent:";
    if(IsNull() || !HasParent())
    {
      ss << "NULL";
    }
    else
    {
      ss << _node->GetRawParentPtr()->GetName() << "(0x" << _node->GetRawParentPtr() << ")";
    }
    
    return ss.str();
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  PoseNd PoseBase<PoseNd,TransformNd>::FindRoot() const
  {
    PoseNd root;
    
    if(IsRoot())
    {
      root.WrapExistingNode(_node);
    }
    else
    {
      root.WrapExistingNode(_node->FindRoot());
    }
    
    return root;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  PoseNd PoseBase<PoseNd,TransformNd>::GetParent() const
  {
    PoseNd parent; // Start out with "null" pose
    if(!IsNull())
    {
      parent.WrapExistingNode(_node->GetParent());
    }
    return parent; // Likely uses RVO so no copy. TODO: Use move semantics instead?
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  bool PoseBase<PoseNd,TransformNd>::GetWithRespectTo(const PoseNd& fromPose, const PoseNd& toPose, PoseNd& P_wrt_other)
  {
    if(!ANKI_VERIFY(!fromPose.IsNull() && !toPose.IsNull() && !P_wrt_other.IsNull(),
                    "PoseBase.GetWithRespectTo.NullInputPose", "FromNull:%s ToNull:%s WrtOtherNull:%s",
                    fromPose.IsNull() ? "Y" : "N", toPose.IsNull() ? "Y" : "N", P_wrt_other.IsNull() ? "Y" : "N"))
    {
      return false;
    }
    
    if(&fromPose == &toPose) {
      // Asked for pose w.r.t. itself. Just return fromPose
      PRINT_NAMED_WARNING("PoseBase.GetWithRespectTo.FromEqualsTo",
                          "Pose w.r.t. itself requested.");
      P_wrt_other = fromPose;
      P_wrt_other.SetParent(toPose);
      return true;
    }

    if(!fromPose._node->HasSameRootAs(*toPose._node))
    {
      // We can't get the transformation between two poses that are not WRT the
      // same root!
      return false;
    }
    
    const PoseTreeNode* from = fromPose._node.get();
    const PoseTreeNode* to   = toPose._node.get();
    
    TransformNd T_from(fromPose._node->GetTransform());
    TransformNd T_to(toPose._node->GetTransform());
    
    // First make sure we are pointing at two nodes of the same tree depth,
    // which is the only way they could possibly share the same _parent.
    // Until that's true, walk the deeper node up until it is at the same
    // depth as the shallower node, keeping track of the total transformation
    // along the way. (NOTE: Only one of the following two while loops should
    // run, depending on which node is deeper in the tree)
    
    int depthDiff = from->GetTreeDepth() - to->GetTreeDepth();
    
    while(depthDiff > 0)
    {
      DEV_ASSERT(from->HasParent(), "PoseBase.GetWithRespectTo.FromParentIsNull");
      
      T_from.PreComposeWith( from->GetParentTransform() );
      from = from->GetRawParentPtr();
      
      if(from->IsChildOf(*to))
      {
        // We bumped into the "to" pose on the way up to the common _parent, so
        // we've got the the chained transform ready to go, and there's no
        // need to walk past the "to" pose, up to the common _parent, and right
        // back down, which would unnecessarily compose two more poses which
        // are the inverse of one another by construction.
        std::swap(P_wrt_other._node->GetTransform(), T_from);
        P_wrt_other.SetParent(toPose);
        return true;
      }
      
      --depthDiff;
    }
    
    while(depthDiff < 0)
    {
      DEV_ASSERT(to->HasParent(), "PoseBase.GetWithRespectTo.ToParentIsNull");
      
      T_to.PreComposeWith( to->GetParentTransform() );
      to = to->GetRawParentPtr();
      
      if(to->IsChildOf(*from)) {
        // We bumped into the "from" pose on the way up to the common _parent,
        // so we've got the the (inverse of the) chained transform ready to
        // go, and there's no need to walk past the "from" pose, up to the
        // common _parent, and right back down, which would unnecessarily
        // compose two more poses which are the inverse of one another by
        // construction.
        T_to.Invert();
        std::swap(P_wrt_other._node->GetTransform(), T_to);
        P_wrt_other.SetParent(toPose);
        return true;
      }
      
      ++depthDiff;
    }
    
    // Sanity check: Treedepths should now match:
    DEV_ASSERT(depthDiff == 0, "PoseBase.GetWithRespectTo.NonZeroDepthDiff");
    DEV_ASSERT(to->GetTreeDepth() == from->GetTreeDepth(), "PoseBase.GetWithRespectTo.TreeDepthMismatch");
    
    // Now that we are pointing to the nodes of the same depth, keep moving up
    // until those nodes have the same _parent, totalling up the transformations
    // along the way
    BOUNDED_WHILE(1000, (from != to) && !to->HasSameParentAs(*from))
    {
      DEV_ASSERT(from->HasParent(), "PoseBase.GetWithRespectTo.FromParentIsNull");
      DEV_ASSERT(to->HasParent(), "PoseBase.GetWithRespectTo.ToParentIsNull");
      
      T_from.PreComposeWith( from->GetParentTransform() );
      T_to.PreComposeWith( to->GetParentTransform() );
      
      to = to->GetRawParentPtr();
      from = from->GetRawParentPtr();
    }
    
    // Now compute the total transformation from this pose, up the "from" path
    // in the tree, to the common ancestor, and back down the "to" side to the
    // final other pose.
    //     P_wrt_other = P_to.inv * P_from;
    P_wrt_other._node->GetTransform() = T_to.GetInverse();
    P_wrt_other._node->GetTransform() *= T_from;
    
    // The Pose we are about to return is w.r.t. the "other" pose provided (that
    // was the whole point of the exercise!), so set its _parent accordingly:
    P_wrt_other.SetParent(toPose);
    
    return true;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  inline bool PoseBase<PoseNd,TransformNd>::AreUnownedParentsAllowed()
  {
    return _areUnownedParentsAllowed;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd, class TransformNd>
  inline void PoseBase<PoseNd,TransformNd>::AllowUnownedParents(bool tf)
  {
    _areUnownedParentsAllowed = tf;
  }
  
  template<class PoseNd, class TransformNd>
  inline bool PoseBase<PoseNd,TransformNd>::IsOwned() const
  {
    if(IsNull())
    {
      return false;
    }
    return _node->IsOwned();
  }
  
  template<class PoseNd, class TransformNd>
  inline uint32_t PoseBase<PoseNd,TransformNd>::GetNodeOwnerCount() const
  {
    if(IsNull())
    {
      return 0;
    }
    return _node->GetOwnerCount();
  }
  
} // namespace Anki

#endif // _ANKICORETECH_MATH_POSEBASE_IMPL_H_
