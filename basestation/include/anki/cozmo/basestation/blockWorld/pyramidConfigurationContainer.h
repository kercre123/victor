/**
 * File: pyramidConfigurationContainer.h
 *
 * Author: Kevin M. Karol
 * Created: 11/2/2016
 *
 * Description: Caches pyramids for BlockConfigurationManager
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef Anki_Cozmo_PyramidConfigurationContainer_H
#define Anki_Cozmo_PyramidConfigurationContainer_H

#include "anki/cozmo/basestation/blockWorld/blockConfigurationContainer.h"

namespace Anki{
namespace Cozmo{

namespace BlockConfigurations{
// forward declaration
class Pyramid;
  
using PyramidPtr = std::shared_ptr<const Pyramid>;
using PyramidWeakPtr = std::weak_ptr<const Pyramid>;

class PyramidConfigurationContainer : public BlockConfigurationContainer
{
public:
  PyramidConfigurationContainer();
  
  // accessors
  const std::vector<PyramidPtr>& GetPyramids() const { return _pyramidCache;}
  std::vector<PyramidWeakPtr> GetWeakPyramids() const;
  
  virtual bool AnyConfigContainsObject(const ObjectID& objectID) const override;
  
protected:
  virtual void SetCurrentCacheAsBackup() override { _backupCache = _pyramidCache;}
  virtual void DeleteBackup() override { _backupCache.clear();}
  virtual ConfigPtrVec  AddAllConfigsWithObjectToCache(const Robot& robot, const ObservableObject* object) override;
  virtual void ClearCache() override;

private:
  std::vector<PyramidPtr> _pyramidCache;
  std::vector<PyramidPtr> _backupCache;
  
}; // class PyramidConfigurationContainer
  
} // namespace BlockConfigurations
} // namespace Cozmo
} // namespace Anki


#endif // Anki_Cozmo_PyramidConfigurationContainer_H

