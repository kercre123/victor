/**
 * File: poseTreeNode.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 8/4/2017
 *
 *
 * Description: Implements the internal tree node for poses, which contains a 2D or 3D transform, a parent pointer,
 *              a name, and an ID.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Common_Math_PoseTreeNode_H__
#define __Anki_Common_Math_PoseTreeNode_H__

#include "util/global/globalDefinitions.h"
#include "util/helpers/boundedWhile.h"
#include "util/logging/logging.h"
#include "coretech/common/engine/math/poseBase.h"

#include <list>
#include <mutex>
#include <set>

// Make a separate define so we can easily dissociate "Dev" pose checks from debug/release/shipping decisions
// (i.e., we may want to switch back to doing them only in Debug)
#if !defined(DO_DEV_POSE_CHECKS)
#if defined(NDEBUG)
#  define DO_DEV_POSE_CHECKS     0
#else
#  define DO_DEV_POSE_CHECKS     1
#endif
#endif

namespace Anki {

template<class PoseNd, class TransformNd>
class PoseBase<PoseNd,TransformNd>::PoseTreeNode
{
public:
  PoseTreeNode(const TransformNd& tform, const std::shared_ptr<PoseTreeNode>& parent, const std::string& name);
  PoseTreeNode(const PoseTreeNode& other);

  ~PoseTreeNode();

  PoseTreeNode& operator=(const PoseTreeNode& other);

  PoseTreeNode(PoseTreeNode&&) = delete;
  PoseTreeNode& operator=(PoseTreeNode&&) = delete;

  // Check various node or tree relationships
  bool IsRoot() const;
  bool HasParent() const { return !IsRoot(); }

  bool IsChildOf(const PoseTreeNode& otherNode) const;
  bool IsParentOf(const PoseTreeNode& otherNode) const;

  bool HasSameParentAs(const PoseTreeNode& other) const;
  bool HasSameRootAs(const PoseTreeNode& other) const;

  PoseID_t GetRootID() const;

  std::string GetNamedPathToRoot(bool showTranslations) const;
  std::shared_ptr<PoseTreeNode> FindRoot() const;

  unsigned int GetTreeDepth() const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  TransformNd&          GetTransform()       { return _transform; }
  const TransformNd&    GetTransform() const { return _transform; }

  // Just get parent's transform without mucking with shared pointers
  const TransformNd&    GetParentTransform() const { return _parentPtr->GetTransform(); }

  const std::shared_ptr<PoseTreeNode>& GetParent() const;
  void                                 SetParent(const std::shared_ptr<PoseTreeNode>& parent);
  const PoseTreeNode*                  GetRawParentPtr() const;

  const std::string&    GetName() const { return _name; }
  void                  SetName(const std::string& name) { _name = name; }

  // Set the pose ID. IDs are not copied with poses.
  void                  SetID(PoseID_t newID) { _id = newID; }

  // Get the ID assigned to this pose. Initial (unset) ID is zero.
  PoseID_t              GetID() const { return _id; }

  // Register/unregister as an "owner" of this node: i.e. that this node is "wrapped" by one or more
  // PoseBase objects (as opposed to simply having a shared_ptr hanging on to it somewhere)
  void      AddOwner()            { ++_ownerCount; }
  void      RemoveOwner();
  bool      IsOwned()       const { return (_ownerCount > 0); }
  uint32_t  GetOwnerCount() const { return _ownerCount; }

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Member Variables
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  TransformNd                     _transform;
  std::shared_ptr<PoseTreeNode>   _parentPtr;
  std::string                     _name;
  PoseID_t                        _id = 0;
  uint32_t                        _ownerCount = 0;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Dev methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // asserts that the parent is valid and that the child is a current valid child of the parent
  static void Dev_AssertIsValidParentPointer(const PoseTreeNode* parent, const PoseTreeNode* childBasePointer);

  // updates children bookkeeping variables for old and new parents
  static void Dev_SwitchParent(const PoseTreeNode* oldParent, const PoseTreeNode* newParent, const PoseTreeNode* childBasePointer);
  // note I'm not alive anymore, I can't be asked about parents
  static void Dev_PoseDestroyed(const PoseTreeNode* basePointer);
  // note I'm alive, and people can ask me about my children
  static void Dev_PoseCreated(const PoseTreeNode* basePointer);

  // funny note: because plugins create PoseNds on load/creation, we can't rely on static variables being initialized,
  // so we need all static variables here to be created on demand, which should be guaranteed by using static local
  // variables returned from a method. I say funny because now it's funny, but believe it was not funny before.

  // static set of valid poses
  using PoseCPtrSet = std::set<const PoseTreeNode*>;
  static PoseCPtrSet& Dev_GetValidPoses() {
    static PoseCPtrSet sDev_ValidPoses;
    return sDev_ValidPoses;
  }

  // mutex because we create poses like bunnies from different threads
  static std::mutex& Dev_GetMutex() {
    static std::mutex sDev_Mutex;
    return sDev_Mutex;
  }

  // bookkeeping variables to ensure parent pointer validity. They should be empty in release.
  // During debug, however, parents keep track of their children so that we can validate proper parenthood
  mutable PoseCPtrSet _devChildrenPtrs; // my current valid children

}; // class PoseTreeNode

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
PoseBase<PoseNd,TransformNd>::PoseTreeNode::PoseTreeNode(const TransformNd& tform,
                                                         const std::shared_ptr<PoseTreeNode>& parent,
                                                         const std::string& name)
: _transform(tform)
, _name(name)
{
  Dev_PoseCreated(this);
  SetParent(parent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
PoseBase<PoseNd,TransformNd>::PoseTreeNode::PoseTreeNode(const PoseTreeNode& other)
: PoseTreeNode(other._transform, other._parentPtr, other._name)
{
  if(!other._name.empty())
  {
    _name = other._name + "_COPY";
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
PoseBase<PoseNd,TransformNd>::PoseTreeNode::~PoseTreeNode()
{
  SetParent(nullptr);

  Dev_PoseDestroyed(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
typename PoseBase<PoseNd,TransformNd>::PoseTreeNode& PoseBase<PoseNd,TransformNd>::PoseTreeNode::operator=(const PoseTreeNode& other)
{
  SetParent(other._parentPtr);
  _transform = other._transform;
  _name = other._name;
  if(!_name.empty())
  {
    _name += "_COPY";
  }
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
bool PoseBase<PoseNd,TransformNd>::PoseTreeNode::IsRoot() const
{
  Dev_AssertIsValidParentPointer(_parentPtr.get(), this);
  return (_parentPtr == nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
bool PoseBase<PoseNd,TransformNd>::PoseTreeNode::IsChildOf(const PoseTreeNode& otherNode) const
{
  if(IsRoot())
  {
    return false;
  }

  const bool isChildOfOther = (_parentPtr.get() == &otherNode);
  if(DO_DEV_POSE_CHECKS && isChildOfOther)
  {
    Dev_AssertIsValidParentPointer(_parentPtr.get(), this);
  }
  return isChildOfOther;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
bool PoseBase<PoseNd,TransformNd>::PoseTreeNode::IsParentOf(const PoseTreeNode& otherNode) const
{
  if(otherNode.IsRoot())
  {
    return false;
  }

  const bool isParentOfOther = (otherNode._parentPtr.get() == this);
  if(DO_DEV_POSE_CHECKS && isParentOfOther)
  {
    Dev_AssertIsValidParentPointer(this, &otherNode);
  }
  return isParentOfOther;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
bool PoseBase<PoseNd,TransformNd>::PoseTreeNode::HasSameParentAs(const PoseTreeNode& other) const
{
  // A node has the same parent as itself, even if it is a root
  if(this == &other)
  {
    return true;
  }

  // Otherwise, a root cannot have the same parent as anything else, by definition
  if(IsRoot() || other.IsRoot())
  {
    return false;
  }

  const bool areParentsSame = (_parentPtr == other._parentPtr);
  return areParentsSame;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
bool PoseBase<PoseNd,TransformNd>::PoseTreeNode::HasSameRootAs(const PoseTreeNode& other) const
{
  // Traverse parents up to root of "this"
  const PoseTreeNode* thisRootPtr = this;
  BOUNDED_WHILE(1000, thisRootPtr->HasParent())
  {
    thisRootPtr = thisRootPtr->_parentPtr.get();
  }

  // Traverse parents up to root of "other"
  const PoseTreeNode* otherRootPtr = &other;
  BOUNDED_WHILE(1000, otherRootPtr->HasParent())
  {
    otherRootPtr = otherRootPtr->_parentPtr.get();
  }

  const bool areRootsSame = (thisRootPtr == otherRootPtr);
  return areRootsSame;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
PoseID_t PoseBase<PoseNd,TransformNd>::PoseTreeNode::GetRootID() const
{
  const PoseTreeNode* rootPtr = this;
  BOUNDED_WHILE(1000, rootPtr->HasParent())
  {
    rootPtr = rootPtr->_parentPtr.get();
  }
  return rootPtr->GetID();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
void PoseBase<PoseNd,TransformNd>::PoseTreeNode::SetParent(const std::shared_ptr<PoseTreeNode>& newParent)
{
  DEV_ASSERT(newParent.get() != this, "PoseBase.PoseTreeNode.SetParent.ParentCannotBeSelf");
  Dev_SwitchParent(_parentPtr.get(), newParent.get(), this);
  _parentPtr = newParent;
  Dev_AssertIsValidParentPointer(_parentPtr.get(), this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
const std::shared_ptr<typename PoseBase<PoseNd,TransformNd>::PoseTreeNode>& PoseBase<PoseNd,TransformNd>::PoseTreeNode::GetParent() const
{
  Dev_AssertIsValidParentPointer(_parentPtr.get(), this);
  return _parentPtr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
const typename PoseBase<PoseNd,TransformNd>::PoseTreeNode* PoseBase<PoseNd,TransformNd>::PoseTreeNode::GetRawParentPtr() const
{
  Dev_AssertIsValidParentPointer(_parentPtr.get(), this);
  return _parentPtr.get();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
std::shared_ptr<typename PoseBase<PoseNd,TransformNd>::PoseTreeNode> PoseBase<PoseNd,TransformNd>::PoseTreeNode::FindRoot() const
{
  // Callers must check for this case and handle it specially!
  DEV_ASSERT(!this->IsRoot(), "PoseBase.PoseTreeNode.FindRoot.ThisIsRoot");

  std::shared_ptr<PoseTreeNode> root = _parentPtr;
  BOUNDED_WHILE(1000, !root->IsRoot())
  {
    // The only way the current root's _parent is null is if it is a
    // root, which means we should have already exited the while loop.
    DEV_ASSERT(root->_parentPtr != nullptr, "PoseBase.PoseTreeNode.FindRoot.RootHasNullParent");
    root = root->GetParent();
  }

  return root;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
std::string PoseBase<PoseNd,TransformNd>::PoseTreeNode::GetNamedPathToRoot(bool showTranslations) const
{
  std::string str;

  const PoseCPtrSet* validPoses = (ANKI_DEV_CHEATS ? &Dev_GetValidPoses() : nullptr);

  const PoseTreeNode* current = this;

  // NOTE: Not using any methods like GetParent() or IsRoot() here because those
  // call Dev_ helpers, which may also be calling this method
  BOUNDED_WHILE(1000, true)
  {
    if(DO_DEV_POSE_CHECKS)
    {
      assert(validPoses != nullptr);
      if(validPoses->find(current) == validPoses->end())
      {
        // Stop as soon as we reach an invalid pose in the chain b/c it may be a garbage pointer
        str += "[INVALID]";
        return str;
      }
    }

    const std::string& name = current->_name;
    if(name.empty()) {
      str += "(UNNAMED)";
    } else {
      str += name;
    }

    if(showTranslations) {
      str += current->_transform.GetTranslation().ToString();
    }

    current = current->_parentPtr.get();

    if(nullptr == current)
    {
      // Reached root (or end of the road anyway)
      break;
    }

    str += " -> ";
  }

  return str;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
unsigned int PoseBase<PoseNd,TransformNd>::PoseTreeNode::GetTreeDepth() const
{
  unsigned int treeDepth = 1;

  const PoseTreeNode* current = this;
  BOUNDED_WHILE(1000, (current->_parentPtr != nullptr))
  {
    ++treeDepth;
    current = current->_parentPtr.get();
  }

  return treeDepth;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
void PoseBase<PoseNd,TransformNd>::PoseTreeNode::Dev_SwitchParent(const PoseTreeNode* oldParent,
                                                                  const PoseTreeNode* newParent,
                                                                  const PoseTreeNode* child)
{
  if ( DO_DEV_POSE_CHECKS )
  {
    std::lock_guard<std::mutex> lock( Dev_GetMutex() );

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
        oldParent->_devChildrenPtrs.erase(child);
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
        ANKI_VERIFY(newParent->_devChildrenPtrs.find(child) == newParent->_devChildrenPtrs.end(),
                   "PoseBase.Dev_SwitchParent.DuplicatedChildOfParent",
                    "Child: '%s'(%p), Old parent '%s'(%p) path to root: %s. "
                    "New parent '%s'(%p) path to root: %s.",
                    child->GetName().c_str(), child,
                    oldParent == nullptr ? "(none)" : oldParent->GetName().c_str(),
                    oldParent,
                    oldParent == nullptr ? "(none)" : oldParent->GetNamedPathToRoot(true).c_str(),
                    newParent->GetName().c_str(), newParent, newParent->GetNamedPathToRoot(true).c_str());

        // add now
        newParent->_devChildrenPtrs.insert(child);
      }
      else
      {
        // this pose can't be a parent, how did you get the pointer?!
        ANKI_VERIFY(false, "PoseBase.Dev_SwitchParent.NewParentIsNotAValidPose",
                    "New parent '%s'(%p) path to root: %s",
                    newParent->GetName().c_str(), newParent,
                    newParent->GetNamedPathToRoot(true).c_str());
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
void PoseBase<PoseNd,TransformNd>::PoseTreeNode::Dev_AssertIsValidParentPointer(const PoseTreeNode* parent,
                                                                                const PoseTreeNode* child)
{
  // Note: (for now?) *always* doing this cheap check, even in shipping, but only aborting in Debug/Release
  if(parent != nullptr && !PoseBase<PoseNd,TransformNd>::AreUnownedParentsAllowed())
  {
    if(!ANKI_VERIFY(parent->_ownerCount > 0,
                    "PoseBase.Dev_AssertIsValidParentPointer.UnownedParent",
                    "Pose %d(%s) has parent %d(%s) which is not owned by any PoseBase wrapper",
                    child->_id, child->_name.c_str(), parent->_id, parent->_name.c_str()))
    {
      if( DO_DEV_POSE_CHECKS )
      {
        Anki::Util::sLogFlush();
        Anki::Util::sAbort();
      }
    }
  }

  if ( DO_DEV_POSE_CHECKS )
  {
    std::lock_guard<std::mutex> lock( Dev_GetMutex() );

    // a) child is a valid pose
    PoseCPtrSet& validPoses = Dev_GetValidPoses();
    if( !ANKI_VERIFY(validPoses.find(child) != validPoses.end(),
                     "PoseBase.Dev_AssertIsValidParentPointer.ChildNotAValidPose",
                     "This pose is bad. Not printing info because it's garbage, so I don't want to access it") )
    {
      Anki::Util::sLogFlush();
      Anki::Util::sAbort();
    }

    if ( nullptr != parent )
    {
      // if we have a parent, check that:
      // b) the parent is a valid pose
      if( !ANKI_VERIFY(validPoses.find(parent) != validPoses.end(),
                  "PoseBase.Dev_AssertIsValidParentPointer.ParentNotAValidPose",
                  "Path of parent '%s'(%p) to root: %s. Path of child '%s'(%p) to root: %s",
                  parent->GetName().c_str(), parent, parent->GetNamedPathToRoot(true).c_str(),
                  child == nullptr ? "(none)" : child->GetName().c_str(), child,
                  child == nullptr ? "(none)" : child->GetNamedPathToRoot(true).c_str()))
      {
        Anki::Util::sLogFlush();
        Anki::Util::sAbort();
      }

      // c) we are a current child of it
      if( !ANKI_VERIFY(parent->_devChildrenPtrs.find(child) != parent->_devChildrenPtrs.end(),
                  "PoseBase.Dev_AssertIsValidParentPointer.ChildOfOldParent",
                  "Path of parent '%s'(%p) to root: %s. Path of child '%s'(%p) to root: %s",
                  parent->GetName().c_str(), parent, parent->GetNamedPathToRoot(true).c_str(),
                  child == nullptr ? "(none)" : child->GetName().c_str(), child,
                  child == nullptr ? "(none)" : child->GetNamedPathToRoot(true).c_str()))
      {
        Anki::Util::sLogFlush();
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
template<class PoseNd, class TransformNd>
void PoseBase<PoseNd,TransformNd>::PoseTreeNode::Dev_PoseDestroyed(const PoseTreeNode* nodePtr)
{
  if ( DO_DEV_POSE_CHECKS )
  {
    std::lock_guard<std::mutex> lock( Dev_GetMutex() );

    // nonsense to ask for nullptr
    assert( nullptr != nodePtr );

    // remove from validPoses and make sure we were a valid pose
    PoseCPtrSet& validPoses = Dev_GetValidPoses();
    const size_t removedCount = validPoses.erase(nodePtr);
    ANKI_VERIFY(removedCount == 1, "PoseBase.Dev_PoseDestroyed.DestroyingInvalidPose",
                "Path from '%s'(%p) to root: %s",
                nodePtr->GetName().c_str(), nodePtr,
                nodePtr->GetNamedPathToRoot(true).c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
void PoseBase<PoseNd,TransformNd>::PoseTreeNode::Dev_PoseCreated(const PoseTreeNode* nodePtr)
{
  if ( DO_DEV_POSE_CHECKS )
  {
    std::lock_guard<std::mutex> lock( Dev_GetMutex() );

    // nonsense to ask for nullptr
    assert( nullptr != nodePtr );

    PoseCPtrSet& validPoses = Dev_GetValidPoses();
    const auto insertRetInfo = validPoses.insert(nodePtr);
    ANKI_VERIFY(insertRetInfo.second, "PoseBase.Dev_PoseCreated.CreatingDuplicatedPointer",
                "Path from '%s'(%p) to root: %s",
                nodePtr->GetName().c_str(), nodePtr,
                nodePtr->GetNamedPathToRoot(true).c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class PoseNd, class TransformNd>
inline void PoseBase<PoseNd,TransformNd>::PoseTreeNode::RemoveOwner()
{
  if(ANKI_VERIFY(_ownerCount > 0, "PoseBase.PoseTreeNode.RemoveOwner.ZeroOwners", ""))
  {
    --_ownerCount;

    // COZMO-10891: We can probably remove this once this ticket is figured out.
    // In our usage, only PoseOrigins in the PoseOriginList have IDs and are named OriginNN.
    // Those origins should never get deleted, so they should always be owned. Flag if this is not the case.
    if(_ownerCount == 0 && !AreUnownedParentsAllowed())
    {
      ANKI_VERIFY(GetID() == 0,
                  "PoseBase.PoseTreeNode.RemoveOwner.ZeroOwnersWithID",
                  "Pose %s has non-zero ID=%d but just had its last owner removed",
                  GetName().c_str(), GetID());
    }
  }
}

} // namespace Anki

#endif /* __Anki_Common_Math_PoseTreeNode_H__ */
