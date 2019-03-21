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

  // TBD: Load string translations
  LOG_INFO("LocaleComponent.InitDependent", "Initialize component");

}

std::string LocaleComponent::GetString(const std::string & stringID) const
{
  // TBD: Perform string translation
  return stringID;
}

} // end namespace Vector
} // end namespace Anki
