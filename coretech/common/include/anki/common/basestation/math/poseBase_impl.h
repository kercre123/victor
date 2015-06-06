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
#include "anki/util/logging/logging.h"
#include "anki/common/basestation/math/poseBase.h"

namespace Anki {

  //template<class PoseNd>
  //std::list<PoseNd> PoseBase<PoseNd>::Origins = {{PoseNd(
  
  //template<class PoseNd>
  //const PoseNd* PoseBase<PoseNd>::_sWorld = &PoseBase<PoseNd>::Origins.front();
  
  /*
  template<class PoseNd>
  PoseNd& PoseBase<PoseNd>::AddOrigin()
  {
    PoseBase<PoseNd>::Origins.emplace_back();
    PoseBase<PoseNd>::Origins.back().SetParent(nullptr);
    PoseBase<PoseNd>::Origins.back().SetName("Origin_" + std::to_string(PoseBase<PoseNd>::Origins.size()));
    
    // TODO: If this gets too long, trigger cleanup?
    
    return PoseBase<PoseNd>::Origins.back();
  }
  
  template<class PoseNd>
  PoseNd& PoseBase<PoseNd>::AddOrigin(const PoseNd &origin)
  {
    if(origin.GetParent() != nullptr) {
      PRINT_NAMED_WARNING("PoseBase.AddOrigin.NonNullParent",
                          "Adding an origin whose parent is non-NULL. This may be ok, but may be a sign of something wrong.\n");
    }
    
    PoseBase<PoseNd>::Origins.emplace_back(origin);
    PoseBase<PoseNd>::Origins.back().SetName("Origin" + std::to_string(PoseBase<PoseNd>::Origins.size()));
    
    // TODO: If this gets too long, trigger cleanup?
    
    return PoseBase<PoseNd>::Origins.back();
  }
  */
  
  template<class PoseNd>
  PoseBase<PoseNd>::PoseBase()
  : PoseBase<PoseNd>(nullptr, "")
  {
    
  }
  
  template<class PoseNd>
  PoseBase<PoseNd>::PoseBase(const PoseNd* parentPose, const std::string& name)
  : _parent(parentPose), _name(name)
  {
    
  }
  
  
  template<class PoseNd>
  const PoseNd& PoseBase<PoseNd>::FindOrigin(const PoseNd& forPose) const
  {
    const PoseNd* originPose = &forPose;
    BOUNDED_WHILE(1000, (!originPose->IsOrigin()))
    {  
      // The only way the current originPose's _parent is null is if it is an
      // origin, which means we should have already exited the while loop.
      CORETECH_ASSERT(originPose->_parent != nullptr);
      
      originPose = originPose->_parent;
    }
    
    return *originPose;
    
  } // FindOrigin()
  
  template<class PoseNd>
  void PoseBase<PoseNd>::PrintNamedPathToOrigin(const PoseNd& startPose, bool showTranslations)
  {
    std::string str = GetNamedPathToOrigin(startPose, showTranslations);
    fprintf(stdout, "%s\n", str.c_str());
  }
  
  template<class PoseNd>
  std::string PoseBase<PoseNd>::GetNamedPathToOrigin(const PoseNd& startPose, bool showTranslations)
  {
    std::string str("Path to origin: ");

    const PoseNd* current = &startPose;
    BOUNDED_WHILE(1000, (!current->IsOrigin()))
    {
      const std::string& name = current->GetName();
      if(name.empty()) {
        str += "(UNNAMED)";
      } else {
        str += name;
      }
      
      if(showTranslations) {
        const Vec3f& T = current->GetTranslation();
        str += "(" + std::to_string(T.x()) + "," + std::to_string(T.y()) + "," + std::to_string(T.z()) + ")";
      }
      
      str += " -> ";
      
      current = current->GetParent();
    }
    
    str += current->GetName();
    
    return str;
    
  } // PrintPathToOrigin()
  
  
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
      PRINT_NAMED_WARNING("PoseBase.GetWithRespectTo.FromEqualsTo", "Pose w.r.t. itself requested.\n");
      P_wrt_other = fromPose;
      P_wrt_other.SetParent(&toPose);
      return true;
    }
    
    if(&fromPose.FindOrigin() != &toPose.FindOrigin()) {
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
      CORETECH_ASSERT(from->_parent != nullptr);
      
      P_from.PreComposeWith( *(from->_parent) );
      from = from->_parent;
      
      if(from->_parent == to) {
        // We bumped into the "to" pose on the way up to the common _parent, so
        // we've got the the chained transform ready to go, and there's no
        // need to walk past the "to" pose, up to the common _parent, and right
        // back down, which would unnecessarily compose two more poses which
        // are the inverse of one another by construction.
        P_from._parent = new_parent;
        P_wrt_other = P_from;
        return true;
      }
      
      --depthDiff;
    }
    
    BOUNDED_WHILE(1000, depthDiff < 0)
    {
      CORETECH_ASSERT(to->_parent != nullptr);
      
      P_to.PreComposeWith( *(to->_parent) );
      to = to->_parent;
      
      if(to->_parent == from) {
        // We bumped into the "from" pose on the way up to the common _parent,
        // so we've got the the (inverse of the) chained transform ready to
        // go, and there's no need to walk past the "from" pose, up to the
        // common _parent, and right back down, which would unnecessarily
        // compose two more poses which are the inverse of one another by
        // construction.
        P_to.Invert();
        P_to._parent = new_parent;
        P_wrt_other = P_to;
        return true;
      }
      
      ++depthDiff;
    }
    
    // Treedepths should now match:
    CORETECH_ASSERT(depthDiff == 0);
    CORETECH_ASSERT(GetTreeDepth(to) == GetTreeDepth(from));
    
    // Now that we are pointing to the nodes of the same depth, keep moving up
    // until those nodes have the same _parent, totalling up the transformations
    // along the way
    BOUNDED_WHILE(1000, to->_parent != from->_parent)
    {
      CORETECH_ASSERT(from->_parent != nullptr && to->_parent != nullptr);
      
      P_from.PreComposeWith( *(from->_parent) );
      P_to.PreComposeWith( *(to->_parent) );
      
      to = to->_parent;
      from = from->_parent;
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
    P_wrt_other._parent = new_parent;
    
    return true;
    
  } // GetWithRespectToHelper()
  
  
} // namespace Anki

#endif // _ANKICORETECH_MATH_POSEBASE_IMPL_H_