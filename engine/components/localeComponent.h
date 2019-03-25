/**
 * File: engine/components/localeComponent.h
 *
 * Description: Container for locale & localization management
 *
 * Copyright: Anki, Inc. 2019
 **/

#ifndef ANKI_ENGINE_LOCALECOMPONENT_H
#define ANKI_ENGINE_LOCALECOMPONENT_H

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "engine/robotComponents_fwd.h"

#include <mutex>
#include <string>
#include <unordered_map>

namespace Anki {
namespace Vector {

class LocaleComponent : public RobotComponent
{
public:
  // Default constructor
  LocaleComponent();

  // Which components do we depend on?
  void GetInitDependencies(RobotCompIDSet & dependencies) const override;

  // Initialize with given dependencies
  void InitDependent(Robot * robot, const RobotCompMap & dependencies) override;

  //
  // Return localized string for given stringID.
  // If localized string is not available, return stringID itself.
  //
  std::string GetString(const std::string & stringID) const;
  std::string GetString(const std::string & stringID, const std::string & arg0) const ;
  std::string GetString(const std::string & stringID, const std::string & arg0, const std::string & arg1) const;

private:
  // Internal types
  using StringMap = std::unordered_map<std::string,std::string>;

  // Map of translations
  StringMap _map;

  // Mutex to protect map
  mutable std::mutex _mutex;

  // Load strings from given file
  bool LoadFile(const std::string & path);

  // Load strings from given directory
  bool LoadDirectory(const std::string & path);

};

} // end namespace Vector
} // end namespace Anki

#endif
