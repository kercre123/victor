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
 * Description: Implements a base class for Pose2d and Pose3d to inherit from in
 *                order to share pose tree capabilities.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef _ANKICORETECH_MATH_POSEBASE_IMPL_H_
#define _ANKICORETECH_MATH_POSEBASE_IMPL_H_

#include "anki/common/basestation/utils/helpers/boundedWhile.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"
#include "anki/common/basestation/math/poseBase.h"

namespace Anki {
 
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd>
  PoseBase<PoseNd>::PoseBase()
  : PoseBase<PoseNd>(nullptr, "") // note this calls another constructor, so do not notify of creation here
  {
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd>
  PoseBase<PoseNd>::PoseBase(const PoseNd* parentPose, const std::string& name)
  : _parentPtr(nullptr)
  , _name(name)
  {
    // valid pose
    Dev_PoseCreated(this);
  
    // use accessors to set parent
    SetParent(parentPose);
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // destructor
  template<class PoseNd>
  PoseBase<PoseNd>::~PoseBase()
  {
    // clear parent to remove reference count
    SetParent(nullptr);
    
    // notify we are destroyed
    Dev_PoseDestroyed(this);
  }
    
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // copy constructor
  template<class PoseNd>
  PoseBase<PoseNd>::PoseBase(const PoseBase& other)
  : _parentPtr(nullptr)
  , _name(other._name)
  {
    // valid pose
    Dev_PoseCreated(this);
  
    // use accessors for parent switch
    SetParent( other.GetParent() );

    // copy rest
    _name = other._name;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // assignment op
  template<class PoseNd>
  PoseBase<PoseNd>& PoseBase<PoseNd>::operator=(const PoseBase& other)
  {
    // use accessors for parent switch
    SetParent( other.GetParent() );
    
    // copy rest and return
    _name = other._name;
    return *this;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd>
  void PoseBase<PoseNd>::SetParent(const PoseNd* otherPose)
  {
    DEV_ASSERT(otherPose != this, "PoseBase.SetParent.ParentCannotBeSelf");
    Dev_SwitchParent(_parentPtr, otherPose, this);
    _parentPtr = otherPose;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  template<class PoseNd>
  const PoseNd& PoseBase<PoseNd>::FindOrigin(const PoseNd& forPose) const
  {
    const PoseNd* originPose = &forPose;
    BOUNDED_WHILE(1000, (!originPose->IsOrigin()))
    {  
      // The only way the current originPose's _parent is null is if it is an
      // origin, which means we should have already exited the while loop.
      DEV_ASSERT(originPose->GetParent() != nullptr, "PoseBase.FindOrigin.OriginHasNullParent");
      originPose = originPose->GetParent();
    }
    
    return *originPose;
    
  } // FindOrigin()
  
  template<class PoseNd>
  void PoseBase<PoseNd>::PrintNamedPathToOrigin(const PoseNd& startPose, bool showTranslations)
  {
    std::string str = GetNamedPathToOrigin(startPose, showTranslations);
    fprintf(stdout, "Path to origin: %s\n", str.c_str());
  }
  
  template<class PoseNd>
  std::string PoseBase<PoseNd>::GetNamedPathToOrigin(const PoseNd& startPose, bool showTranslations)
  {
    std::string str;

    const PoseCPtrSet* validPoses = (ANKI_DEV_CHEATS ? &Dev_GetValidPoses() : nullptr);
    
    const PoseNd* current = &startPose;

    // NOTE: Not using any methods like GetParent() or IsOrigin() here because those
    // call Dev_ helpers, which may also be calling this method
    BOUNDED_WHILE(1000, true)
    {
      if(ANKI_DEV_CHEATS)
      {
        assert(validPoses != nullptr);
        if(validPoses->find(current) == validPoses->end())
        {
          // Stop as soon as we reach an invalid pose in the chain b/c it may be a garbage pointer
          str += "[INVALID]";
          return str;
        }
      }
      
      const std::string& name = current->GetName();
      if(name.empty()) {
        str += "(UNNAMED)";
      } else {
        str += name;
      }
      
      if(showTranslations) {
        str += current->GetTranslation().ToString();
      }
      
      current = current->_parentPtr;
      
      if(nullptr == current)
      {
        // Reached origin (or end of the road anyway)
        break;
      }
      
      str += " -> ";
    }
    
    return str;
    
  } // GetNamedPathToOrigin()
  
  
  // Count number of steps to an origin node, by walking up
  // the chain of _parents.
  template<class PoseNd>
  unsigned int PoseBase<PoseNd>::GetTreeDepth(const PoseNd* poseNd) const
  {
    unsigned int treeDepth = 1;
    
    const PoseNd* current = poseNd;
    BOUNDED_WHILE(1000, (!current->IsOrigin()))
    {
      ++treeDepth;
      current = current->GetParent();
    }
    
    return treeDepth;
  }
  
  template<class PoseNd>
  bool PoseBase<PoseNd>::GetWithRespectTo(const PoseNd& fromPose, const PoseNd& toPose,
                                          PoseNd& P_wrt_other) const
  {
    if(&fromPose == &toPose) {
      // Asked for pose w.r.t. itself. Just return fromPose
      PRINT_NAMED_WARNING("PoseBase.GetWithRespectTo.FromEqualsTo",
                          "Pose w.r.t. itself requested.");
      P_wrt_other = fromPose;
      P_wrt_other.SetParent(&toPose);
      return true;
    }
    
    const PoseBase<PoseNd>* fromOrigin = &fromPose.FindOrigin();
    const PoseBase<PoseNd>* toOrigin   = &toPose.FindOrigin();
    if(fromOrigin != toOrigin)
    {
      // We can get the transformation between two poses that are not WRT the
      // same origin!
      return false;
    }
    
    const PoseNd* from = &fromPose;
    const PoseNd* to   = &toPose;
    
    PoseNd P_from(fromPose);
    
    // "to" can get changed below, but we want to set the returned pose's
    // _parent to it, so we keep a copy here.
    const PoseNd *new_parent = to;
    
    /*
     if(to == Pose<DIM>::World) {
     
     // Special (but common!) case: get with respect to the
     // world pose.  Just chain together poses up to the root.
     
     while(from->_parent != POSE::World) {
     P_from.PreComposeWith( *(from->_parent) );
     from = from->_parent;
     }
     
     P_wrt_other = &P_from;
     
     } else {
     */
    PoseNd P_to(toPose);
    
    // First make sure we are pointing at two nodes of the same tree depth,
    // which is the only way they could possibly share the same _parent.
    // Until that's true, walk the deeper node up until it is at the same
    // depth as the shallower node, keeping track of the total transformation
    // along the way. (NOTE: Only one of the following two while loops should
    // run, depending on which node is deeper in the tree)
    
    int depthDiff = GetTreeDepth(from) - GetTreeDepth(to);
    
    BOUNDED_WHILE(1000, depthDiff > 0)
    {
      DEV_ASSERT(from->GetParent() != nullptr, "PoseBase.GetWithRespectTo.FromParentIsNull");
      
      P_from.PreComposeWith( *(from->GetParent()) );
      from = from->GetParent();
      
      if(from->GetParent() == to) {
        // We bumped into the "to" pose on the way up to the common _parent, so
        // we've got the the chained transform ready to go, and there's no
        // need to walk past the "to" pose, up to the common _parent, and right
        // back down, which would unnecessarily compose two more poses which
        // are the inverse of one another by construction.
        P_from.SetParent(new_parent);
        P_wrt_other = P_from;
        return true;
      }
      
      --depthDiff;
    }
    
    BOUNDED_WHILE(1000, depthDiff < 0)
    {
      DEV_ASSERT(to->GetParent() != nullptr, "PoseBase.GetWithRespectTo.ToParentIsNull");
      
      P_to.PreComposeWith( *(to->GetParent()) );
      to = to->GetParent();
      
      if(to->GetParent() == from) {
        // We bumped into the "from" pose on the way up to the common _parent,
        // so we've got the the (inverse of the) chained transform ready to
        // go, and there's no need to walk past the "from" pose, up to the
        // common _parent, and right back down, which would unnecessarily
        // compose two more poses which are the inverse of one another by
        // construction.
        P_to.Invert();
        P_to.SetParent(new_parent);
        P_wrt_other = P_to;
        return true;
      }
      
      ++depthDiff;
    }
    
    if(ANKI_DEVELOPER_CODE)
    {
      // Sanity check: Treedepths should now match:
      DEV_ASSERT(depthDiff == 0, "PoseBase.GetWithRespectTo.NonZeroDepthDiff");
      DEV_ASSERT(GetTreeDepth(to) == GetTreeDepth(from), "PoseBase.GetWithRespectTo.TreeDepthMismatch");
    }
    
    // Now that we are pointing to the nodes of the same depth, keep moving up
    // until those nodes have the same _parent, totalling up the transformations
    // along the way
    BOUNDED_WHILE(1000, to->GetParent() != from->GetParent())
    {
      DEV_ASSERT(from->GetParent() != nullptr, "PoseBase.GetWithRespectTo.FromParentIsNull");
      DEV_ASSERT(to->GetParent() != nullptr, "PoseBase.GetWithRespectTo.ToParentIsNull");
      
      P_from.PreComposeWith( *(from->GetParent()) );
      P_to.PreComposeWith( *(to->GetParent()) );
      
      to = to->GetParent();
      from = from->GetParent();
    }
    
    // Now compute the total transformation from this pose, up the "from" path
    // in the tree, to the common ancestor, and back down the "to" side to the
    // final other pose.
    //     P_wrt_other = P_to.inv * P_from;
    P_wrt_other = P_to.GetInverse();
    P_wrt_other *= P_from;
    
    // } // IF/ELSE other is the World pose
    
    // The Pose we are about to return is w.r.t. the "other" pose provided (that
    // was the whole point of the exercise!), so set its _parent accordingly:
    P_wrt_other.SetParent(new_parent);
    
    return true;
    
  } // GetWithRespectToHelper()
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd>
  void PoseBase<PoseNd>::Dev_SwitchParent(const PoseNd* oldParent, const PoseNd* newParent, const PoseBase<PoseNd>* childBasePointer)
  {
    if ( ANKI_DEV_CHEATS )
    {
      std::lock_guard<std::mutex> lock( Dev_GetMutex() );
      
      const PoseNd* castedChild = reinterpret_cast<const PoseNd*>(childBasePointer);
      PoseCPtrSet& validPoses = Dev_GetValidPoses();
    
      // if there's an old parent, we have to tell it we are not a child anymore
      if ( oldParent )
      {
        // if the parent is not a valid pose anymore it's not an error, we had a bad pointer but we are not using it,
        // we are actually switching to a different one
        auto match = validPoses.find(oldParent);
        if ( match != validPoses.end() )
        {
          // if it's still a valid parent, we notify
          const PoseBase<PoseNd>* upCastedParent = reinterpret_cast<const PoseBase<PoseNd>*>(oldParent);
          upCastedParent->_devChildrenPtrs.erase(castedChild);
        }
      }
    
      // if we have a new parent, add ourselves as a child
      if ( newParent )
      {
        // check if it can be a parent
        auto match = validPoses.find(newParent);
        if ( match != validPoses.end() )
        {
          // make sure we were not there before
          ANKI_VERIFY(newParent->_devChildrenPtrs.find(castedChild) == newParent->_devChildrenPtrs.end(),
                     "PoseBase.Dev_SwitchParent.DuplicatedChildOfParent",
                      "Child: '%s'(%p), Old parent '%s'(%p) path to origin: %s. "
                      "New parent '%s'(%p) path to origin: %s.",
                      childBasePointer->GetName().c_str(), childBasePointer,
                      oldParent == nullptr ? "(none)" : oldParent->GetName().c_str(),
                      oldParent,
                      oldParent == nullptr ? "(none)" : GetNamedPathToOrigin(*oldParent, true).c_str(),
                      newParent->GetName().c_str(), newParent, GetNamedPathToOrigin(*newParent, true).c_str());
          
          // add now
          const PoseBase<PoseNd>* upCastedParent = reinterpret_cast<const PoseBase<PoseNd>*>(newParent);
          upCastedParent->_devChildrenPtrs.insert(castedChild);
        }
        else
        {
          // this pose can't be a parent, how did you get the pointer?!
          ANKI_VERIFY(false, "PoseBase.Dev_SwitchParent.NewParentIsNotAValidPose",
                      "New parent '%s'(%p) path to origin: %s",
                      newParent->GetName().c_str(), newParent,
                      GetNamedPathToOrigin(*newParent, true).c_str());
        }
      }
    }
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd>
  void PoseBase<PoseNd>::Dev_AssertIsValidParentPointer(const PoseNd* parent, const PoseBase<PoseNd>* childBasePointer)
  {
    if ( ANKI_DEV_CHEATS )
    {
      std::lock_guard<std::mutex> lock( Dev_GetMutex() );
      const PoseNd* downCastedChild = reinterpret_cast<const PoseNd*>(childBasePointer);

      // a) child is a valid pose
      PoseCPtrSet& validPoses = Dev_GetValidPoses();
      if( !ANKI_VERIFY(validPoses.find(downCastedChild) != validPoses.end(),
                       "PoseBase.Dev_AssertIsValidParentPointer.ChildNotAValidPose",
                       "This pose is bad. Not printing info because it's garbage, so I don't want to access it") )
      {
        Anki::Util::sAbort();
      }
  
      if ( nullptr != parent )
      {
        // if we have a parent, check that:
        // b) the parent is a valid pose
        if( !ANKI_VERIFY(validPoses.find(parent) != validPoses.end(),
                    "PoseBase.Dev_AssertIsValidParentPointer.ParentNotAValidPose",
                    "Path of parent '%s'(%p) to origin: %s. Path of child '%s'(%p) to origin: %s",
                    parent->GetName().c_str(), parent, GetNamedPathToOrigin(*parent, true).c_str(),
                    childBasePointer == nullptr ? "(none)" : childBasePointer->GetName().c_str(), childBasePointer,
                    childBasePointer == nullptr ? "(none)" : GetNamedPathToOrigin(*reinterpret_cast<const PoseNd*>(childBasePointer), true).c_str()))
        {
          Anki::Util::sAbort();
        }
        
        // c) we are a current child of it
        const PoseBase<PoseNd>* upCastedParent = reinterpret_cast<const PoseBase<PoseNd>*>(parent);
        if( !ANKI_VERIFY(upCastedParent->_devChildrenPtrs.find(downCastedChild) != upCastedParent->_devChildrenPtrs.end(),
                    "PoseBase.Dev_AssertIsValidParentPointer.ChildOfOldParent",
                    "Path of parent '%s'(%p) to origin: %s. Path of child '%s'(%p) to origin: %s",
                    parent->GetName().c_str(), parent, GetNamedPathToOrigin(*parent, true).c_str(),
                    childBasePointer == nullptr ? "(none)" : childBasePointer->GetName().c_str(), childBasePointer,
                    childBasePointer == nullptr ? "(none)" : GetNamedPathToOrigin(*downCastedChild, true).c_str()))
        {
          Anki::Util::sAbort();
        }
        
        // Note on c): If you crash on c), it means that the child is pointing at this parent, and this parent
        // is indeed a valid pose, but it is not a valid parent of this child. This can happen by this series of steps:
        // Create pose A
        // Create pose B
        // Set A as B's parent
        // Destroy A - note B's parent is a bad pointer
        // Create C (C could get allocated in A's released pointer)
        // Create D
        // Set C as D's parent
        // Try to use B's parent.
        // -> In that case B's pointer says that it's a valid pose, however is not a valid child of C,
        // B did not intend to point to C, and using B's parent pointer is an error
      }
    }
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd>
  void PoseBase<PoseNd>::Dev_PoseDestroyed(const PoseBase<PoseNd>* basePointer)
  {
    if ( ANKI_DEV_CHEATS )
    {
      std::lock_guard<std::mutex> lock( Dev_GetMutex() );
    
      // nonsense to ask for nullptr
      assert( nullptr != basePointer );

      // remove from validPoses and make sure we were a valid pose
      PoseCPtrSet& validPoses = Dev_GetValidPoses();
      const PoseNd* castedSelf = reinterpret_cast<const PoseNd*>(basePointer);
      const size_t removedCount = validPoses.erase(castedSelf);
      ANKI_VERIFY(removedCount == 1, "PoseBase.Dev_PoseDestroyed.DestroyingInvalidPose",
                  "Path from '%s'(%p) to origin: %s",
                  basePointer->GetName().c_str(), basePointer,
                  GetNamedPathToOrigin(*reinterpret_cast<const PoseNd*>(basePointer), true).c_str());
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<class PoseNd>
  void PoseBase<PoseNd>::Dev_PoseCreated(const PoseBase<PoseNd>* basePointer)
  {
    if ( ANKI_DEV_CHEATS )
    {
      std::lock_guard<std::mutex> lock( Dev_GetMutex() );
    
      // nonsense to ask for nullptr
      assert( nullptr != basePointer );
     
      PoseCPtrSet& validPoses = Dev_GetValidPoses();
      const PoseNd* castedSelf = reinterpret_cast<const PoseNd*>(basePointer);
      const auto insertRetInfo = validPoses.insert(castedSelf);
      ANKI_VERIFY(insertRetInfo.second, "PoseBase.Dev_PoseCreated.CreatingDuplicatedPointer",
                  "Path from '%s'(%p) to origin: %s",
                  basePointer->GetName().c_str(), basePointer,
                  GetNamedPathToOrigin(*reinterpret_cast<const PoseNd*>(basePointer), true).c_str());
    }
  }
  
  
} // namespace Anki

#endif // _ANKICORETECH_MATH_POSEBASE_IMPL_H_
