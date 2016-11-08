/**
 * File: stackConfigurationContainer.h
 *
 * Author: Kevin M. Karol
 * Created: 11/2/2016
 *
 * Description: Caches stacks for BlockConfigurationManager
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef Anki_Cozmo_StackConfigurationContainer_H
#define Anki_Cozmo_StackConfigurationContainer_H

#include "anki/cozmo/basestation/blockWorld/blockConfigurationContainer.h"

namespace Anki{
namespace Cozmo{
  
namespace BlockConfigurations{
// forward declaration
class StackOfCubes;
  
using StackPtr = std::shared_ptr<const StackOfCubes>;
using StackWeakPtr = std::weak_ptr<const StackOfCubes>;

class StackConfigurationContainer : public BlockConfigurationContainer
{
public:
  StackConfigurationContainer();
  
  // accessors
  const std::vector<StackPtr>& GetStacks() const { return _stackCache;}
  std::vector<StackWeakPtr> GetWeakStacks() const;
  
  // Return a pointer to the current tallest stack
  const StackWeakPtr GetTallestStack() const;
  // Pass in a list of bottom blocks to ignore if you are looking to locate a specific stack
  const StackWeakPtr GetTallestStack(const std::vector<ObjectID>& baseBlocksToIgnore) const;
  
  virtual bool AnyConfigContainsObject(const ObjectID& objectID) const override;

protected:
  virtual void SetCurrentCacheAsBackup() override { _backupCache = _stackCache;}
  virtual void DeleteBackup() override { _backupCache.clear();}
  virtual ConfigPtrVec AddAllConfigsWithObjectToCache(const Robot& robot, const ObservableObject* object) override;
  virtual void ClearCache() override;
  
private:
  std::vector<StackPtr> _stackCache;
  std::vector<StackPtr> _backupCache;
  
}; // class StackConfigurationContainer
  
} // namespace BlockConfigurations
} // namespace Cozmo
} // namespace Anki


#endif // Anki_Cozmo_StackConfigurationContainer_H

