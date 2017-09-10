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

void CozmoExperiments::WriteLabAssignmentsToRobot(const std::vector<Util::AnkiLab::AssignmentDef>& assignments)
{
  LabAssignments labAssignments;
  for (const auto& assignment : assignments)
  {
    LabAssignment labAssignment(assignment.GetExperiment_key(), assignment.GetVariation_key());
    labAssignments.labAssignments.push_back(std::move(labAssignment));
  }

  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  if (robot != nullptr)
  {
    std::vector<u8> assignmentsVec(labAssignments.Size());
    labAssignments.Pack(assignmentsVec.data(), labAssignments.Size());
    if (!robot->GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_LabAssignments,
                                              assignmentsVec.data(), assignmentsVec.size()))
    {
      PRINT_NAMED_ERROR("CozmoExperiments.WriteLabAssignmentsToRobot.Failed", "Write failed");
    }
  }
}

void CozmoExperiments::ReadLabAssignmentsFromRobot(const u32 serialNumber)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  if (robot != nullptr)
  {
    if (!robot->GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_LabAssignments,
                                             [this, serialNumber](u8* data, size_t size, NVStorage::NVResult res)
                                             {
                                               (void) RestoreLoadedActiveExperiments(data, size, res, serialNumber);
                                             }))
    {
      PRINT_NAMED_ERROR("CozmoExperiments.ReadLabAssignmentsFromRobot.Failed", "Read failed");
    }
  }
}

bool CozmoExperiments::RestoreLoadedActiveExperiments(const u8* data, const size_t size,
                                                      const NVStorage::NVResult res, u32 serialNumber)
{
  if (res < NVStorage::NVResult::NV_OKAY)
  {
    // The tag doesn't exist on the robot indicating the robot is new or has been wiped
    if (res == NVStorage::NVResult::NV_NOT_FOUND)
    {
      PRINT_CH_INFO("Unnamed", "CozmoExperiments.RestoreLoadedActiveExperiments",
                    "No lab assignments data on robot");
    }
    else
    {
      PRINT_NAMED_ERROR("NeedsManager.FinishReadFromRobot.ReadFailedFinish", "Read failed with %s", EnumToString(res));
    }
    return false;
  }

  LabAssignments labAssignments;
  labAssignments.Unpack(data, size);

  // We've just loaded any active assignments from the robot; so now apply them
  using namespace Util::AnkiLab;
  AnkiLab& lab = GetAnkiLab();

  const std::string userId = std::to_string(serialNumber);

  for (const auto& assignment : labAssignments.labAssignments)
  {
    (void) lab.RestoreActiveExperiment(assignment.experiment_key, userId, assignment.variation_key);
  }
  return true;
}

}
}
