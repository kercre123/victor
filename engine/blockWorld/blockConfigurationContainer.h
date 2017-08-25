/**
 * File: blockConfigurationContainer.h
 *
 * Author: Kevin M. Karol
 * Created: 11/2/2016
 *
 * Description: Abstract class for caches of configurations that can be maintained
 * and updated by the BlockConfigurationManager
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef Anki_Cozmo_BlockConfigurationContainer_H
#define Anki_Cozmo_BlockConfigurationContainer_H

#include "util/helpers/templateHelpers.h"
#include <vector>

namespace Anki{
// Forward declarations
class ObjectID;
namespace Cozmo{
  
class Robot;
class ObservableObject;
  
namespace BlockConfigurations{
class BlockConfiguration;
class BlockConfigurationManager;

#define CAST_TO_BLOCK_CONFIG_SHARED(x) std::static_pointer_cast<const BlockConfiguration>(x)
using ConfigPtrVec = std::vector<std::shared_ptr<const BlockConfiguration>>;
  
class BlockConfigurationContainer
{
public:
  friend BlockConfigurationManager;
  BlockConfigurationContainer();
  
  // Return true if any stored configs contain the object
  virtual bool AnyConfigContainsObject(const ObjectID& objectID) const = 0;
  virtual int ConfigurationCount() const = 0;
  
protected:
  // Update contains the logic that backs up and then clears the current cache
  // requests that derived classes add all configs with specified objects to the cache
  // and the removes the cache backup to delete shared pointers that have fallen out
  // of scope
  void Update(const Robot& robot);
  
  // Notify the derived class that the current cache is about to be cleared
  // and should be backed up to preserve smart pointers
  virtual void SetCurrentCacheAsBackup()= 0;
  // Notify the derived class that the cache has been updated and the backup
  // should be cleared to destroy lingering shared_pointers
  virtual void DeleteBackup() = 0;
  
  // Notify the derived class that it should clear its cache
  virtual void ClearCache() = 0;
  // Returns a list of all configurations added to the cache
  virtual ConfigPtrVec AddAllConfigsWithObjectToCache(const Robot& robot, const ObservableObject* object) = 0;

  // Helper function derived classes can use to return a subset of configurations
  // that contain an object ID
  template<typename T>
  T AllConfigsWithID(const ObjectID& objectID, const T& configVector);
  
  // ensures that existing shared pointers are returned if available
  // in most cases the currentList passed in should be the cacheBackup
  // newEntry should always be considered invalid after this function is called
  // since it has either been deleted or will be managed by a shared_ptr
  template<typename SharedPointerType, typename ConfigType>
  static SharedPointerType CastNewEntryToBestSharedPointer(const std::vector<SharedPointerType>& currentList, const ConfigType* &newEntry);
  
}; // class BlockConfigurationContainer



template<typename SharedPointerType, typename ConfigType>
SharedPointerType BlockConfigurationContainer::CastNewEntryToBestSharedPointer(const std::vector<SharedPointerType>& currentList, const ConfigType* &newEntry)
{
  for(const auto& entry: currentList){
    if((*entry) == (*newEntry)){
      Util::SafeDelete(newEntry);
      return entry;
    }
  }
  
  auto explicitCast = SharedPointerType(newEntry);
  return explicitCast;
}
  

} // namespace BlockConfigurations
} // namespace Cozmo
} // namespace Anki


#endif // Anki_Cozmo_BlockConfigurationContainer_H
