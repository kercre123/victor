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

#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/aiComponent/aiComponents_fwd.h"
#include "coretech/common/engine/objectIDs.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
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
  PickUpObjectNoAxisCheck = 0,
  // only pick up upright objects, unless rolling is locked (in which case, pick up any object)
  PickUpObjectAxisCheck,
  
  StackBottomObjectAxisCheck,
  StackBottomObjectNoAxisCheck,
  StackTopObjectAxisCheck,
  StackTopObjectNoAxisCheck,
  
  RollObjectWithDelegateNoAxisCheck,
  RollObjectWithDelegateAxisCheck,
  
  PopAWheelieOnObject,
  
  Count
};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ObjectInteractionCache - Maintains a map of ObjectInteractionIntentions
// to the Cache entrys that wrap information about the best objects to use
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class  ObjectInteractionInfoCache : public IDependencyManagedComponent<AIComponentID>, 
                                    private Util::noncopyable  
{
public:
  ObjectInteractionInfoCache(const Robot& robot);
  using BestObjectFunction = std::function<ObjectID(const std::set<ObjectID>& validObjects)>;

  ObjectID GetBestObjectForIntention(ObjectInteractionIntention intention);
  std::set<ObjectID> GetValidObjectsForIntention(ObjectInteractionIntention intention);
  const BlockWorldFilter& GetDefaultFilterForIntention(ObjectInteractionIntention intention);
  
  bool IsObjectValidForInteraction(ObjectInteractionIntention intention, const ObjectID& object);
  // Ensure that all information in the cache that is necessary to get valid/best
  // objects has been updated this tick before accessing it
  void EnsureInformationValid(ObjectInteractionIntention intention);
  
protected:

  using FullValidInteractionArray = Util::FullEnumToValueArrayChecker::FullEnumToValueArray
     <ObjectInteractionIntention, BlockWorldFilter*, ObjectInteractionIntention::Count>;
  using FullBestInteractionArray = Util::FullEnumToValueArrayChecker::FullEnumToValueArray
     <ObjectInteractionIntention, BestObjectFunction, ObjectInteractionIntention::Count>;
  
  void ConfigureObjectInteractionFilters(const Robot& robot,
                                         FullValidInteractionArray validInteractions,
                                         FullBestInteractionArray  bestInteractions);
  
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
  
  // Common logic for checking validity of blocks for any Pickup, PopAWheelie, or Roll Interactions
  bool CanPickupNoAxisCheck(const ObservableObject* object) const;
  bool CanPickupAxisCheck(const ObservableObject* object) const;
  bool CanUseAsStackTopNoAxisCheck(const ObservableObject* object) const;
  bool CanUseAsStackTopAxisCheck(const ObservableObject* object) const;
  bool CanUseAsStackBottomNoAxisCheck(const ObservableObject* object);
  bool CanUseAsStackBottomAxisCheck(const ObservableObject* object);
  bool CanUseAsStackBottomHelper(const ObservableObject* object,
                                 ObjectInteractionIntention intention);
  bool CanUseForPopAWheelie(const ObservableObject* object) const;
  bool CanRollObjectDelegateNoAxisCheck(const ObservableObject* object) const;
  bool CanRollObjectDelegateAxisCheck(const ObservableObject* object) const;
  
  // Functions which return the "best" object to use given a set of valid objects
  ObjectID DefaultBestObjectFunction(const std::set<ObjectID>& validObjects);
  ObjectID RollBlockBestObjectFunction(const std::set<ObjectID>& validObjects);
};
    
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ObjectInteractionCacheEntry - wraps information about the which blocks are
// valid and best to use for certain intentions, and insure this information is
// up to date before returning it to the requester
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class  ObjectInteractionCacheEntry{
public:
  using BestObjectFunction = ObjectInteractionInfoCache::BestObjectFunction;
  ObjectInteractionCacheEntry(const Robot& robot,
                              const std::string& debugName,
                              BlockWorldFilter* validFilter,
                              BestObjectFunction bestFilter);
  ObjectID GetBestObject() const;
  std::set< ObjectID > GetValidObjects() const;
  const BlockWorldFilter& GetValidObjectsFilter() const {assert(_blockWorldFilterValidBlocks);
    return *_blockWorldFilterValidBlocks;}
  
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
  std::unique_ptr< BlockWorldFilter > _blockWorldFilterValidBlocks;
  BestObjectFunction                  _bestObjFunc;
  float                               _timeUpdated_s;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_ObjectInteractionInfoCache_H__
