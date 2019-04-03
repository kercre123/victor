/**
 * File: engine/components/localeComponent.cpp
 *
 * Description: Container for locale management & localization methods
 *
 * Copyright: Anki, Inc. 2019
 **/

#include "engine/components/localeComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "util/environment/locale.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/string/stringUtils.h"

#define LOG_CHANNEL "Locale"

namespace Anki {
namespace Vector {

LocaleComponent::LocaleComponent() : RobotComponent(this, RobotComponentID::LocaleComponent)
{

}

void LocaleComponent::GetInitDependencies(RobotCompIDSet & dependencies) const
{
  dependencies.insert(RobotComponentID::CozmoContextWrapper);
}

void LocaleComponent::InitDependent(Robot *, const RobotCompMap & dependencies)
{
  // Validate preconditions
  const auto & contextWrapper = dependencies.GetComponent<ContextWrapper>();
  const auto * context = contextWrapper.GetContext();
  DEV_ASSERT(context != nullptr, "LocaleComponent.InitDependent.InvalidContext");

  const auto * dataPlatform = context->GetDataPlatform();
  DEV_ASSERT(dataPlatform != nullptr, "LocaleComponent.InitDependent.InvalidDataPlatform");

  const auto * locale = context->GetLocale();
  DEV_ASSERT(locale != nullptr, "LocaleComponent.InitDependent.InvalidLocale");

  const std::string & resourcePath = dataPlatform->GetResourcePath("assets/LocalizedStrings");
  const std::string & localePath = resourcePath + "/" + locale->ToString();

  //
  // Try to load strings from current locale.  If that fails, fall back to default locale.
  //
  if (!LoadDirectory(localePath)) {
    LOG_WARNING("LocaleComponent.InitDependent", "Unable to load %s", localePath.c_str());
    LoadDirectory(resourcePath + "/" + Util::Locale::kDefaultLocale.ToString());
  }

}

bool LocaleComponent::LoadFile(const std::string & path)
{
  LOG_INFO("LocaleComponent.LoadFile", "Load %s", path.c_str());

  try {
    const std::string & document = Util::FileUtils::ReadFile(path);

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(document, root)) {
      const auto & errors = reader.getFormattedErrorMessages();
      LOG_WARNING("LocaleComponent.LoadFile", "Unable to parse %s (%s)", path.c_str(), errors.c_str());
      return false;
    }

    if (!root.isObject()) {
      LOG_WARNING("LocaleComponent.LoadFile", "Invalid root object in %s", path.c_str());
      return false;
    }

    const auto & memberNames = root.getMemberNames();

    // Lock the map while we add entries
    std::lock_guard<std::mutex> lock(_mutex);

    for (const auto & memberName : memberNames) {
      if (memberName == "smartling") {
        // Skip translation directives
        continue;
      }

      const auto & member = root[memberName];
      if (!member.isObject()) {
        LOG_WARNING("LocaleComponent.LoadFile", "Invalid entry for key %s", memberName.c_str());
        continue;
      }

      const auto & translation = member["translation"];
      if (!translation.isString()) {
        LOG_WARNING("LocaleComponent.LoadFile", "Invalid translation for key %s", memberName.c_str());
        continue;
      }

      _map[memberName] = translation.asString();
    }

  } catch (const std::exception & ex) {
    LOG_ERROR("LocaleComponent.LoadFile", "Failed to load %s (%s)", path.c_str(), ex.what());
    return false;
  }

  return true;
}

bool LocaleComponent::LoadDirectory(const std::string & path)
{
  LOG_INFO("LocaleComponent.LoadDirectory", "Load %s", path.c_str());

  if (!Util::FileUtils::DirectoryExists(path)) {
    LOG_WARNING("LocaleComponent.LoadDirectory", "Invalid directory %s", path.c_str());
    return false;
  }

  bool ok = true;

  const auto & files = Util::FileUtils::FilesInDirectory(path, true);
  for (const auto & file : files) {
    if (!LoadFile(file)) {
      LOG_WARNING("LocaleComponent.LoadDirectory", "Failed to load %s", file.c_str());
      ok = false;
    }
  }
  return ok;
}

std::string LocaleComponent::GetString(const std::string & stringID) const
{
  // Lock the map while we search for an entry
  std::lock_guard<std::mutex> lock(_mutex);
  const auto & pos = _map.find(stringID);
  if (pos != _map.end()) {
    return pos->second;
  }
  return stringID;
}

std::string LocaleComponent::GetString(const std::string & stringID,
                                       const std::string & arg0) const
{
  std::string s = GetString(stringID);
  Anki::Util::StringReplace(s, "{0}", arg0);
  return s;
}

std::string LocaleComponent::GetString(const std::string & stringID,
                                       const std::string & arg0,
                                       const std::string & arg1) const
{
  std::string s = GetString(stringID);
  Anki::Util::StringReplace(s, "{0}", arg0);
  Anki::Util::StringReplace(s, "{1}", arg1);
  return s;

}

} // end namespace Vector
} // end namespace Anki
