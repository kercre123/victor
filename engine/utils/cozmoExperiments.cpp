/**
 * File: cozmoExperiments
 *
 * Author: baustin
 * Created: 8/3/17
 *
 * Description: Interface into A/B test system
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/utils/cozmoExperiments.h"

#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotManager.h"
#include "util/ankiLab/extLabInterface.h"
#if USE_DAS
#include <DAS/DAS.h>
#include <DAS/DASPlatform.h>
#endif

namespace Anki {
namespace Cozmo {

CozmoExperiments::CozmoExperiments(const CozmoContext* context)
: _context(context)
, _tags(context)
{
}

static const char* GetDeviceId()
{
#if USE_DAS
  return DASGetPlatform()->GetDeviceId();
#else
  return "user"; // non-empty string keeps it from failing on mac release
#endif
}

void CozmoExperiments::InitExperiments()
{
  _lab.Enable(!Util::AnkiLab::ShouldABTestingBeDisabled());

  AutoActivateExperiments(GetDeviceId());

  (void) _tags.VerifyTags(_lab.GetKnownAudienceTags());

  // Provide external A/B interface what it needs to work with Cozmo
  auto labOpRunner = [this] (const Util::AnkiLab::LabOperation& op) {
    op(&_lab);
  };
  auto userIdAccessor = [this] {
    Robot* robot = _context->GetRobotManager()->GetFirstRobot();
    return robot != nullptr ? std::to_string(robot->GetBodySerialNumber()) : GetDeviceId();
  };
  Util::AnkiLab::InitializeABInterface(labOpRunner, userIdAccessor);
}

Util::AnkiLab::AssignmentStatus CozmoExperiments::ActivateExperiment(
  const Util::AnkiLab::ActivateExperimentRequest& request, std::string& outVariationKey)
{
  std::string deviceId;
  const std::string* userIdString = &request.user_id;
  if (request.user_id.empty()) {
    userIdString = &deviceId;
    deviceId = GetDeviceId();
  }
  const auto& tags = _tags.GetQualifiedTags();
  return _lab.ActivateExperiment(request.experiment_key, *userIdString, tags, outVariationKey);
}

void CozmoExperiments::AutoActivateExperiments(const std::string& userId)
{
  const auto& tags = _tags.GetQualifiedTags();
  (void) _lab.AutoActivateExperimentsForUser(userId, tags);
}

}
}
