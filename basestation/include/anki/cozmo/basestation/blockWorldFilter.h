/**
 * File: blockWorldFilter.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 10/27/2015
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description: A helper class for filtering searches through objects in BlockWorld.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Anki_Cozmo_BlockWorldFilter_H__
#define __Anki_Cozmo_BlockWorldFilter_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/cozmoObservableObject.h"
#include "clad/types/objectFamilies.h"
#include "clad/types/objectTypes.h"

#include <set>
#include <assert.h>


namespace Anki {
namespace Cozmo {

  // Forward declaration
  class ObservableObject;
  
  class BlockWorldFilter
  {
  public:

    BlockWorldFilter() { }
    
    // These are the methods called by BlockWorld when looping over existing
    // object families, types, and IDs to decide whether to continue.
    // An object cannot be in an ignore list and either the allowed list must be
    // empty or the object must be in it in order to pass.
    bool ConsiderFamily(ObjectFamily family) const;
    bool ConsiderType(ObjectType type) const;
    bool ConsiderObject(ObservableObject* object) const; // Checks ID and runs FilterFcn(object)
    
    // Set the entire set of IDs, types, or families to ignore in one go.
    void SetIgnoreIDs(std::set<ObjectID>&& IDs);
    void SetIgnoreTypes(std::set<ObjectType>&& types);
    void SetIgnoreFamilies(std::set<ObjectFamily>&& families);
    
    // Add to the existing set of IDs, types, or families
    void AddIgnoreID(ObjectID ID);
    void AddIgnoreIDs(std::set<ObjectID>&& IDs);
    void AddIgnoreType(ObjectType type);
    void AddIgnoreFamily(ObjectFamily family);
    
    // Set the entire set of IDs, types, or families to be allowed in one go.
    void SetAllowedIDs(std::set<ObjectID>&& IDs);
    void SetAllowedTypes(std::set<ObjectType>&& types);
    void SetAllowedFamilies(std::set<ObjectFamily>&& families);
    
    // Add to the existing set of IDs, types, or families
    void AddAllowedID(ObjectID ID);
    void AddAllowedIDs(std::set<ObjectID>&& IDs);
    void AddAllowedType(ObjectType type);
    void AddAllowedFamily(ObjectFamily family);
    
    // Set the filtering function used at the object level
    using FilterFcn = std::function<bool(ObservableObject*)>;
    void SetFilterFcn(FilterFcn filterFcn);
    
    // Normally, all objects known to BlockWorld are checked. Setting this to
    // true will only check those objects observed in the most recent BlockWorld
    // Update() call.
    void OnlyConsiderLatestUpdate(bool tf) { _onlyConsiderLatestUpdate = tf; }
    bool IsOnlyConsideringLatestUpdate() const { return _onlyConsiderLatestUpdate; }
    
  protected:
    std::set<ObjectID>      _ignoreIDs, _allowedIDs;
    std::set<ObjectType>    _ignoreTypes, _allowedTypes;
    std::set<ObjectFamily>  _ignoreFamilies, _allowedFamilies;
    
    FilterFcn _filterFcn = &BlockWorldFilter::DefaultFilterFcn;
    
    bool _onlyConsiderLatestUpdate = false;
      
    // The default filter function should be overriden if the poseState will be unknown or other functionality is desired
    static bool DefaultFilterFcn(ObservableObject* object) { assert(nullptr != object); return !object->IsPoseStateUnknown(); }
    
  }; // class BlockWorldFilter
  
  
# pragma mark - Inlined Implementations
  
  inline void BlockWorldFilter::SetIgnoreFamilies(std::set<ObjectFamily> &&families) {
    _ignoreFamilies = families;
  }
  
  inline void BlockWorldFilter::SetIgnoreTypes(std::set<ObjectType> &&types) {
    _ignoreTypes = types;
  }
  
  inline void BlockWorldFilter::SetIgnoreIDs(std::set<ObjectID> &&IDs) {
    _ignoreIDs = IDs;
  }
  
  inline void BlockWorldFilter::SetAllowedIDs(std::set<ObjectID>&& IDs) {
    _allowedIDs = IDs;
  }
  
  inline void BlockWorldFilter::SetAllowedTypes(std::set<ObjectType>&& types) {
    _allowedTypes = types;
  }
  
  inline void BlockWorldFilter::SetAllowedFamilies(std::set<ObjectFamily>&& families) {
    _allowedFamilies = families;
  }
  
  inline void BlockWorldFilter::SetFilterFcn(FilterFcn filterFcn) {
    _filterFcn = filterFcn;
  }
  
  inline void BlockWorldFilter::AddIgnoreID(ObjectID ID) {
    _ignoreIDs.insert(ID);
  }
  
  inline void BlockWorldFilter::AddIgnoreIDs(std::set<ObjectID> &&IDs) {
    _ignoreIDs.insert(IDs.begin(), IDs.end());
  }
  
  inline void BlockWorldFilter::AddIgnoreType(ObjectType type) {
    _ignoreTypes.insert(type);
  }
  
  inline void BlockWorldFilter::AddIgnoreFamily(ObjectFamily family) {
    _ignoreFamilies.insert(family);
  }
  
  inline void BlockWorldFilter::AddAllowedID(ObjectID ID) {
    _allowedIDs.insert(ID);
  }

  inline void BlockWorldFilter::AddAllowedIDs(std::set<ObjectID>&& IDs) {
    _allowedIDs.insert(IDs.begin(), IDs.end());
  }
  
  inline void BlockWorldFilter::AddAllowedType(ObjectType type) {
    _allowedTypes.insert(type);
  }
  
  inline void BlockWorldFilter::AddAllowedFamily(ObjectFamily family) {
    _allowedFamilies.insert(family);
  }

  inline bool BlockWorldFilter::ConsiderFamily(ObjectFamily family) const {
    return (_ignoreFamilies.count(family)==0 && (_allowedFamilies.empty() || _allowedFamilies.count(family)>0));
  }
  
  inline bool BlockWorldFilter::ConsiderType(ObjectType type) const {
    return (_ignoreTypes.count(type)==0 && (_allowedTypes.empty() || _allowedTypes.count(type)>0));
  }
  
  inline bool BlockWorldFilter::ConsiderObject(ObservableObject* object) const {
    return (_ignoreIDs.count(object->GetID())==0 &&
            (_allowedIDs.empty() || _allowedIDs.count(object->GetID())>0) &&
            _filterFcn(object));
  }

} // namespace Cozmo
} // namespace Anki



#endif // __Anki_Cozmo_BlockWorldFilter_H__
