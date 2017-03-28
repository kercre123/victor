/**
 * File: objectInteractionInfoCache.h
 *
 * Author: Kevin M. Karol
 * Created: 04/27/17
 *
 * Description: Cache which keeps track of the valid and best objects to use
 * with certain object interaction intentions.  Updated lazily for performance
 * improvements
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_ObjectInteractionInfoCache_H__
#define __Cozmo_Basestation_BehaviorSystem_ObjectInteractionInfoCache_H__

#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"

#include "anki/common/basestation/objectIDs.h"

#include "util/helpers/fullEnumToValueArrayChecker.h"

#include <assert.h>
#include <set>
#include <unordered_map>

namespace Anki {
namespace Cozmo {
  
class ObjectInteractionCacheEntry;
class ObservableObject;
class Robot;

// intention of what to do with an object. Add things here to centralize logic for the best object to use
enum class ObjectInteractionIntention {
  // any object which can be picked up
  PickUpAnyObject = 0,
  // only pick up upright objects, unless rolling is locked (in which case, pick up any object)
  PickUpObjectWithAxisCheck,
  
  RollObjectWithDelegateNoAxisCheck,
  RollObjectWithDelegateAxisCheck,
  
  PopAWheelieOnObject,
  
  PyramidBaseObject,
  PyramidStaticObject,
  PyramidTopObject,
  
  Count
};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ObjectInteractionCache - Maintains a map of ObjectInteractionIntentions
// to the Cache entrys that wrap information about the best objects to use
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class  ObjectInteractionInfoCache{
public:
  ObjectInteractionInfoCache(const Robot& robot);
  

  ObjectID GetBestObjectForIntention(ObjectInteractionIntention intention);
  std::set<ObjectID> GetValidObjectsForIntention(ObjectInteractionIntention intention);
  const BlockWorldFilter& GetDefaultFilterForIntention(ObjectInteractionIntention intention);
  
  bool IsObjectValidForInteraction(ObjectInteractionIntention intention, const ObjectID& object);
  // Ensure that all information in the cache that is necessary to get valid/best
  // objects has been updated this tick before accessing it
  void EnsureInformationValid(ObjectInteractionIntention intention);
  
protected:
  using FullInteractionArray = Util::FullEnumToValueArrayChecker::FullEnumToValueArray
     <ObjectInteractionIntention, BlockWorldFilter*, ObjectInteractionIntention::Count>;
  
  void ConfigureObjectInteractionFilters(const Robot& robot, FullInteractionArray interactions);
  
  // Allows the whiteboard to notify us of taps/invalidations
  friend class AIWhiteboard;
  // Notify the cache of an object tap so that it can update the best object to
  // use for each InteractionIntention
  void ObjectTapInteractionOccurred(const ObjectID& objectID);
  
  // Force all InteractionIntentions to update their maps the next time
  // accessing the best or valid objects occurs
  void InvalidateAllIntents();

  
  
private:
  const Robot& _robot;
  std::unordered_map<ObjectInteractionIntention,
                         ObjectInteractionCacheEntry> _trackers;
  
  static const char* ObjectUseIntentionToString(ObjectInteractionIntention intention);
  
  // Common logic for checking validity of blocks for any Pickup, PopAWheelie, or Roll action
  bool CanPickupHelper(const ObservableObject* object) const;
  bool CanPickupWithAxisHelper(const ObservableObject* object) const;
  bool CanPopAWheelieHelper(const ObservableObject* object) const;
  bool CanRollObjectDelegateNoAxisHelper(const ObservableObject* object) const;
  bool CanRollObjectDelegateWithAxisHelper(const ObservableObject* object) const;
  bool CanUseAsBuildPyramidBaseBlock(const ObservableObject* object) const;
  bool CanUseAsBuildPyramidStaticBlock(const ObservableObject* object);
  bool CanUseAsBuildPyramidTopBlock(const ObservableObject* object);
};
    
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ObjectInteractionCacheEntry - wraps information about the which blocks are
// valid and best to use for certain intentions, and insure this information is
// up to date before returning it to the requester
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class  ObjectInteractionCacheEntry{
public:
  ObjectInteractionCacheEntry(const Robot& robot,
                              const std::string& debugName,
                              BlockWorldFilter* filter);
  ObjectID GetBestObject() const;
  std::set< ObjectID > GetValidObjects() const;
  const BlockWorldFilter& GetFilter() const {assert(_blockWorldFilter);
    return *_blockWorldFilter;}
  
  // Updates the bestObject and validObjects if they haven't been updated this tick
  void EnsureInformationValid();
  // Notify the cache that a tap has been detected on an object
  // Function returns true if the filter can use the object that was tapped, false otherwise
  bool ObjectTapInteractionOccurred(const ObjectID& objectID);
  
  // Marks the current best object and valid objects invalid and forces
  // an update of them on the next EnsureInformationValid()
  void Invalidate();
  
private:
  const Robot&                        _robot;
  std::string                         _debugName;
  ObjectID                            _bestObject;
  std::set< ObjectID >                _validObjects;
  std::unique_ptr< BlockWorldFilter > _blockWorldFilter;
  float                               _timeUpdated_s;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_ObjectInteractionInfoCache_H__
