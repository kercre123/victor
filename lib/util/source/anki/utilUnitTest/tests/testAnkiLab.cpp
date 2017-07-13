/**
 * File: testAnkiLab.cpp
 *
 * Author: chapados
 * Created: 1/25/17
 *
 * Test suite for AnkiLab (A/B Testing) core framework.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "utilUnitTest/tests/testFileHelper.h"
#include "util/fileUtils/fileUtils.h"
#include "util/string/stringUtils.h"

#include "util/ankiLab/ankiLab.h"
#include "util/ankiLab/ankiLabAccessors.h"

#include <sys/stat.h>


namespace Anki {
namespace Util {
namespace AnkiLab {

class AnkiLabTest : public FileHelper
{
public:
  AnkiLab CreateAnkiLabAB50() const;
  const std::string CreateExperimentAB50DataJsonContents() const;

protected:
  virtual void SetUp() override {
  }

  virtual void TearDown() override {
  }
};

AnkiLab AnkiLabTest::CreateAnkiLabAB50() const
{
  const std::string experimentsJsonContents = CreateExperimentAB50DataJsonContents();

  AnkiLab lab;
  (void)lab.Load(experimentsJsonContents);
  return lab;
}

const std::string AnkiLabTest::CreateExperimentAB50DataJsonContents() const
{
  return R"json(
  {
    "meta": {
      "project_id": "od",
      "version": 0,
      "revision": 0
    },
    "experiments": [
                    {
                    "key": "report_test_all_control",
                    "version": 0,
                    "activation_mode": "manual",
                    "start_time_utc_iso8601": "",
                    "stop_time_utc_iso8601": "",
                    "pause_time_utc_iso8601": "",
                    "resume_time_utc_iso8601": "",
                    "pop_frac_pct": 100,
                    "variations": [
                                   {
                                   "key": "control",
                                   "pop_frac_pct": 100
                                   },
                                   {
                                   "key": "treatment",
                                   "pop_frac_pct": 0
                                   }
                                   ],
                    "audience_tags": [],
                    "forced_variations": [
                                          { "user_id": "1234", "variation_key": "treatment" },
                                          { "user_id": "5678", "variation_key": "control" }
                                          ]
                    },
                    {
                    "key": "report_test_all_treatment",
                    "version": 0,
                    "activation_mode": "manual",
                    "start_time_utc_iso8601": "",
                    "stop_time_utc_iso8601": "",
                    "pause_time_utc_iso8601": "",
                    "resume_time_utc_iso8601": "",
                    "pop_frac_pct": 100,
                    "variations": [
                                   {
                                   "key": "control",
                                   "pop_frac_pct": 0
                                   },
                                   {
                                   "key": "treatment",
                                   "pop_frac_pct": 100
                                   }
                                   ],
                    "audience_tags": [],
                    "forced_variations": [
                                          { "user_id": "1234", "variation_key": "treatment" },
                                          { "user_id": "5678", "variation_key": "control" }
                                          ]
                    },
                    {
                    "key": "report_test",
                    "version": 0,
                    "activation_mode": "manual",
                    "start_time_utc_iso8601": "",
                    "stop_time_utc_iso8601": "",
                    "pause_time_utc_iso8601": "",
                    "resume_time_utc_iso8601": "",
                    "pop_frac_pct": 50,
                    "variations": [
                                   {
                                   "key": "control",
                                   "pop_frac_pct": 50
                                   },
                                   {
                                   "key": "treatment",
                                   "pop_frac_pct": 50
                                   }
                                   ],
                    "audience_tags": [],
                    "forced_variations": [
                                          { "user_id": "1234", "variation_key": "treatment" },
                                          { "user_id": "5678", "variation_key": "control" }
                                          ]
                    },
                    {
                    "key": "report_test_auto",
                    "version": 0,
                    "activation_mode": "automatic",
                    "start_time_utc_iso8601": "",
                    "stop_time_utc_iso8601": "",
                    "pause_time_utc_iso8601": "",
                    "resume_time_utc_iso8601": "",
                    "pop_frac_pct": 70,
                    "variations": [
                                   {
                                   "key": "control",
                                   "pop_frac_pct": 35
                                   },
                                   {
                                   "key": "treatment",
                                   "pop_frac_pct": 65
                                   }
                                   ],
                    "audience_tags": [],
                    "forced_variations": [
                                          { "user_id": "1234", "variation_key": "treatment" },
                                          { "user_id": "5678", "variation_key": "control" }
                                          ]
                    },
                    {
                    "key": "report_test_first_time_user",
                    "version": 0,
                    "activation_mode": "automatic",
                    "start_time_utc_iso8601": "",
                    "stop_time_utc_iso8601": "",
                    "pause_time_utc_iso8601": "",
                    "resume_time_utc_iso8601": "",
                    "pop_frac_pct": 50,
                    "variations": [
                                   {
                                   "key": "control",
                                   "pop_frac_pct": 50
                                   },
                                   {
                                   "key": "treatment",
                                   "pop_frac_pct": 50
                                   }
                                   ],
                    "audience_tags": [
                                      "first_time_user"
                                      ],
                    "forced_variations": [
                                          { "user_id": "1234", "variation_key": "treatment" },
                                          { "user_id": "5678", "variation_key": "control" }
                                          ]
                    },
                    {
                    "key": "report_test_times",
                    "version": 0,
                    "activation_mode": "automatic",
                    "start_time_utc_iso8601": "2017-04-01T00:00:00Z",
                    "stop_time_utc_iso8601": "2017-04-30T00:00:00Z",
                    "pause_time_utc_iso8601": "2017-04-10T00:00:00Z",
                    "resume_time_utc_iso8601": "2017-04-20T00:00:00Z",
                    "pop_frac_pct": 50,
                    "variations": [
                                   {
                                   "key": "control",
                                   "pop_frac_pct": 50
                                   },
                                   {
                                   "key": "treatment",
                                   "pop_frac_pct": 50
                                   }
                                   ],
                    "audience_tags": [],
                    "forced_variations": [
                                          { "user_id": "1234", "variation_key": "treatment" },
                                          { "user_id": "5678", "variation_key": "control" }
                                          ]
                    }
                    ]
  }
  )json";
}

TEST_F(AnkiLabTest, LoadData)
{
  const std::string experimentsJsonContents = CreateExperimentAB50DataJsonContents();

  AnkiLab lab;
  bool success = lab.Load(experimentsJsonContents);
  EXPECT_TRUE(success);

  const AnkiLabDef& def = lab.GetAnkiLabDefinition();
  EXPECT_EQ(6, def.GetExperiments().size());
}

TEST_F(AnkiLabTest, ExperimentHashBucket)
{
  uint8_t bucket = CalculateExperimentHashBucket("report_test",
                                                        "user_id_0");
  EXPECT_GE(bucket, 0);
  EXPECT_LT(bucket, 100);
}

TEST_F(AnkiLabTest, TestaudienceTagIntersections)
{
  std::vector<std::string> experimentKeys = { "a", "f", "c" };
  std::vector<std::string> audienceTags = { "c", "b", "a", "d", "f", "e" };
  EXPECT_TRUE(AudienceListIsSubsetOfList(experimentKeys, audienceTags));

  std::vector<std::string> mismatchedaudienceTags = { "f", "d", "e", "c" };
  EXPECT_FALSE(AudienceListIsSubsetOfList(experimentKeys, mismatchedaudienceTags));

  // Experiment ids empty
  EXPECT_TRUE(AudienceListIsSubsetOfList(std::vector<std::string>{}, audienceTags));

  // audience ids empty
  EXPECT_FALSE(AudienceListIsSubsetOfList(experimentKeys, std::vector<std::string>{}));
}

TEST_F(AnkiLabTest, TestEpochFromUTCDateString)
{
  time_t epochTs;

  epochTs = Util::EpochSecFromIso8601UTCDateString("1970-01-01T00:00:00Z");
  EXPECT_EQ(0, epochTs);

  epochTs = Util::EpochSecFromIso8601UTCDateString("1970-01-01T00:00:01Z");
  EXPECT_EQ(1, epochTs);

  epochTs = Util::EpochSecFromIso8601UTCDateString("1970-01-01T01:00:00Z");
  EXPECT_EQ(3600, epochTs);

  epochTs = Util::EpochSecFromIso8601UTCDateString("2017-02-23T08:27:51Z");
  EXPECT_EQ(1487838471, epochTs);

#ifndef LINUX // below has a different result on linux

  // I've got 99 seconds, but a `time_t` ain't one.
  epochTs = Util::EpochSecFromIso8601UTCDateString("1970-01-01T00:00:99Z");

  // Invalid date returns UINT32_MAX
  EXPECT_EQ(UINT32_MAX, epochTs);
#endif
}

TEST_F(AnkiLabTest, GetExperimentVariation)
{
  const std::string experimentsJsonContents = CreateExperimentAB50DataJsonContents();

  AnkiLab lab = CreateAnkiLabAB50();

  const Experiment* experiment = lab.FindExperiment("report_test");
  EXPECT_NE(nullptr, experiment);

  {
    const ExperimentVariation* variation = GetExperimentVariation(experiment, 0);
    EXPECT_NE(nullptr, variation);
    EXPECT_EQ("control", variation->GetKey());
  }

  {
    const ExperimentVariation* variation = GetExperimentVariation(experiment, 49);
    EXPECT_EQ(nullptr, variation);
  }

  {
    const ExperimentVariation* variation = GetExperimentVariation(experiment, 99);
    EXPECT_EQ(nullptr, variation);
  }

  {
    const ExperimentVariation* variation = GetExperimentVariation(experiment, 50);
    EXPECT_NE(nullptr, variation);
    EXPECT_EQ("treatment", variation->GetKey());
  }
}

TEST_F(AnkiLabTest, ActivateExperiment)
{
  AnkiLab lab = CreateAnkiLabAB50();

  {
    std::string variationKey;
    AssignmentStatus status = lab.ActivateExperiment("report_test",
                                                     "user_id_8",
                                                     std::vector<std::string>(0),
                                                     variationKey);
    EXPECT_EQ(AssignmentStatus::Assigned, status);
    EXPECT_EQ("treatment", variationKey);
  }

  {
    std::string variationKey;
    AssignmentStatus status = lab.ActivateExperiment("report_test",
                                                     "user_id_0",
                                                     std::vector<std::string>(0),
                                                     variationKey);
    EXPECT_EQ(AssignmentStatus::Assigned, status);
    EXPECT_EQ("control", variationKey);
  }

  {
    std::string variationKey;
    AssignmentStatus status = lab.ActivateExperiment("report_test",
                                                     "user_id_000000",
                                                     std::vector<std::string>(0),
                                                     variationKey);
    EXPECT_EQ(AssignmentStatus::Unassigned, status);
  }
}

TEST_F(AnkiLabTest, ActivateExperimentAllControl)
{
  // Ensure that an experiment with 0% treatment, 100% control is assigned correctly
  AnkiLab lab = CreateAnkiLabAB50();

  const Experiment* experiment = lab.FindExperiment("report_test_all_control");
  EXPECT_NE(nullptr, experiment);
  {
    const ExperimentVariation* variation = GetExperimentVariation(experiment, 0);
    EXPECT_NE(nullptr, variation);
    EXPECT_EQ("control", variation->GetKey());
  }

  {
    const ExperimentVariation* variation = GetExperimentVariation(experiment, 99);
    EXPECT_NE(nullptr, variation);
    EXPECT_EQ("control", variation->GetKey());
  }

  {
    std::string variationKey;
    AssignmentStatus status = lab.ActivateExperiment("report_test_all_control",
                                                     "user_id_8",
                                                     std::vector<std::string>(0),
                                                     variationKey);
    EXPECT_EQ(AssignmentStatus::Assigned, status);
    EXPECT_EQ("control", variationKey);
  }
}

TEST_F(AnkiLabTest, ActivateExperimentAllTreatment)
{
  // Ensure that an experiment with 0% control, 100% treatment is assigned correctly
  AnkiLab lab = CreateAnkiLabAB50();

  const Experiment* experiment = lab.FindExperiment("report_test_all_treatment");
  EXPECT_NE(nullptr, experiment);
  {
    const ExperimentVariation* variation = GetExperimentVariation(experiment, 0);
    EXPECT_NE(nullptr, variation);
    EXPECT_EQ("treatment", variation->GetKey());
  }

  {
    const ExperimentVariation* variation = GetExperimentVariation(experiment, 99);
    EXPECT_NE(nullptr, variation);
    EXPECT_EQ("treatment", variation->GetKey());
  }

  {
    std::string variationKey;
    AssignmentStatus status = lab.ActivateExperiment("report_test_all_treatment",
                                                     "user_id_8",
                                                     {},
                                                     variationKey);
    EXPECT_EQ(AssignmentStatus::Assigned, status);
    EXPECT_EQ("treatment", variationKey);
  }
}

TEST_F(AnkiLabTest, AutoActivateExperiments)
{
  AnkiLab lab = CreateAnkiLabAB50();

  // Should attempt to activate 3 experiments (activation_mode:automatic)
  size_t attemptCount = lab.AutoActivateExperimentsForUser("user_id_8", {"first_time_user"});
  EXPECT_EQ(3, attemptCount);
}

TEST_F(AnkiLabTest, ActivateExperimentWithAudienceFiltering)
{

  {
    // Experiment audienceTags match user audienceTags
    AnkiLab lab = CreateAnkiLabAB50();
    std::string variationKey;
    std::vector<std::string> audienceTags = {"first_time_user"};
    AssignmentStatus status = lab.ActivateExperiment("report_test_first_time_user",
                                                     "user_id_0",
                                                     audienceTags,
                                                     variationKey);
    EXPECT_EQ(AssignmentStatus::Assigned, status);
    EXPECT_EQ("treatment", variationKey);
  }

  {
    // Experiment has audienceTags, user set is empty
    AnkiLab lab = CreateAnkiLabAB50();
    std::string variationKey;
    AssignmentStatus status = lab.ActivateExperiment("report_test_first_time_user",
                                                     "user_id_0",
                                                     std::vector<std::string>(0),
                                                     variationKey);
    EXPECT_EQ(AssignmentStatus::AudienceMismatch, status);
  }

  {
    // Experiment audienceTags do not match user audienceTags
    AnkiLab lab = CreateAnkiLabAB50();
    std::string variationKey;
    std::vector<std::string> audienceTags = {"country_de"};
    AssignmentStatus status = lab.ActivateExperiment("report_test_first_time_user",
                                                     "user_id_0",
                                                     audienceTags,
                                                     variationKey);
    EXPECT_EQ(AssignmentStatus::AudienceMismatch, status);
  }

  {
    // Experiment does not have any audienceTags
    AnkiLab lab = CreateAnkiLabAB50();
    std::string variationKey;
    std::vector<std::string> audienceTags = {"country_de"};
    AssignmentStatus status = lab.ActivateExperiment("report_test",
                                                     "user_id_8",
                                                     audienceTags,
                                                     variationKey);
    EXPECT_EQ(AssignmentStatus::Assigned, status);
    EXPECT_EQ("treatment", variationKey);
  }
}

TEST_F(AnkiLabTest, ActivateExperimentIsRunning) {
  AnkiLab lab = CreateAnkiLabAB50();

  // Test that time points are accurate
  {
    uint32_t epochSec = Util::EpochSecFromIso8601UTCDateString("2017-04-01T00:00:00Z");

    std::string variationKey;
    AssignmentStatus status;
    status = lab.ActivateExperiment("report_test_times",
                                    "user_id_8",
                                    std::vector<std::string>(0),
                                    epochSec,
                                    variationKey);
    EXPECT_EQ(AssignmentStatus::Assigned, status);
    EXPECT_EQ("treatment", variationKey);
  }
}

TEST_F(AnkiLabTest, ActivateExperimentBeforeStarted)
{
  AnkiLab lab = CreateAnkiLabAB50();

  {
    uint32_t epochSec = Util::EpochSecFromIso8601UTCDateString("2017-03-01T00:00:00Z");

    std::string variationKey;
    AssignmentStatus status;
    status = lab.ActivateExperiment("report_test_times",
                                    "user_id_8",
                                    std::vector<std::string>(0),
                                    epochSec,
                                    variationKey);
    EXPECT_EQ(AssignmentStatus::ExperimentNotRunning, status);
  }
}

TEST_F(AnkiLabTest, ActivateExperimentAfterStopped)
{
  AnkiLab lab = CreateAnkiLabAB50();

  {
    uint32_t epochSec = Util::EpochSecFromIso8601UTCDateString("2017-05-01T00:00:00Z");

    std::string variationKey;
    AssignmentStatus status;
    status = lab.ActivateExperiment("report_test_times",
                                    "user_id_8",
                                    std::vector<std::string>(0),
                                    epochSec,
                                    variationKey);
    EXPECT_EQ(AssignmentStatus::ExperimentNotRunning, status);
  }
}

TEST_F(AnkiLabTest, ActivateExperimentWhilePaused)
{
  AnkiLab lab = CreateAnkiLabAB50();

  {
    uint32_t epochSec = Util::EpochSecFromIso8601UTCDateString("2017-04-11T00:00:00Z");

    std::string variationKey;
    AssignmentStatus status;
    status = lab.ActivateExperiment("report_test_times",
                                    "user_id_8",
                                    std::vector<std::string>(0),
                                    epochSec,
                                    variationKey);
    EXPECT_EQ(AssignmentStatus::ExperimentNotRunning, status);
  }
}

TEST_F(AnkiLabTest, ActivateExperimentAfterResume)
{
  AnkiLab lab = CreateAnkiLabAB50();

  {
    uint32_t epochSec = Util::EpochSecFromIso8601UTCDateString("2017-04-21T00:00:00Z");

    std::string variationKey;
    AssignmentStatus status;
    status = lab.ActivateExperiment("report_test_times",
                                    "user_id_8",
                                    std::vector<std::string>(0),
                                    epochSec,
                                    variationKey);
    EXPECT_EQ(AssignmentStatus::Assigned, status);
  }
}

TEST_F(AnkiLabTest, ForceActivateExperiment)
{
  AnkiLab lab = CreateAnkiLabAB50();

  std::string variationKey;
  AssignmentStatus status;
  status = lab.ForceActivateExperiment("report_test",
                                       "user_id_8",
                                       "control");
  EXPECT_EQ(AssignmentStatus::ForceAssigned, status);
}

TEST_F(AnkiLabTest, ForceActivateExperimentFailsForInvalidVariant)
{
  AnkiLab lab = CreateAnkiLabAB50();

  std::string variationKey;
  AssignmentStatus status;
  status = lab.ForceActivateExperiment("report_test",
                                       "user_id_8",
                                       "not_a_valid_variant");
  EXPECT_EQ(AssignmentStatus::VariantNotFound, status);
}

TEST_F(AnkiLabTest, ActivateExperimentOverrideAssignment)
{
  AnkiLab lab = CreateAnkiLabAB50();

  bool willOverride = lab.OverrideExperimentVariation("report_test", "user_id_8", "control");
  EXPECT_TRUE(willOverride);

  std::string variationKey;
  AssignmentStatus status;

  {
    status = lab.ActivateExperiment("report_test",
                                    "user_id_8",
                                    std::vector<std::string>(0),
                                    variationKey);
    EXPECT_EQ(AssignmentStatus::OverrideAssigned, status);
    EXPECT_EQ("control", variationKey);
  }
}

TEST_F(AnkiLabTest, ActivateExperimentOverrideAssignmentRequiresValidCriteria)
{
  AnkiLab lab = CreateAnkiLabAB50();

  bool willOverride = lab.OverrideExperimentVariation("report_test_times", "user_id_8", "control");
  EXPECT_TRUE(willOverride);

  {
    uint32_t epochSec = Util::EpochSecFromIso8601UTCDateString("2017-03-01T00:00:00Z");

    std::string variationKey;
    AssignmentStatus status;
    status = lab.ActivateExperiment("report_test_times",
                                    "user_id_8",
                                    std::vector<std::string>(0),
                                    epochSec,
                                    variationKey);
    EXPECT_EQ(AssignmentStatus::ExperimentNotRunning, status);
  }
}

TEST_F(AnkiLabTest, ActivateExperimentFailsToOverrideActiveAssignment)
{
  AnkiLab lab = CreateAnkiLabAB50();

  {
    // activate experiment
    std::string variationKey;
    AssignmentStatus status;
    status = lab.ActivateExperiment("report_test",
                                    "user_id_8",
                                    std::vector<std::string>(0),
                                    variationKey);
    EXPECT_EQ(AssignmentStatus::Assigned, status);
    EXPECT_EQ("treatment", variationKey);
  }

  // Active Experiment variant can't be overridden
  bool willOverride = lab.OverrideExperimentVariation("report_test", "user_id_8", "control");
  EXPECT_FALSE(willOverride);
}

TEST_F(AnkiLabTest, DisableTheLab)
{
  AnkiLab lab = CreateAnkiLabAB50();
  EXPECT_TRUE(lab.IsEnabled());
  lab.Enable(false);
  EXPECT_FALSE(lab.IsEnabled());

  // Verify that user is unassigned when lab is disabled
  {
    std::string variationKey = "treatment";
    AssignmentStatus status = lab.ActivateExperiment("report_test",
                                                     "user_id_8",
                                                     {},
                                                     variationKey);
    EXPECT_EQ(AssignmentStatus::Unassigned, status);
    EXPECT_EQ("", variationKey);
  }

  // Verify that override is disabled when lab is disabled
  bool willOverride = lab.OverrideExperimentVariation("report_test", "user_id_8", "control");
  EXPECT_FALSE(willOverride);

  // Verify that force is disabled when lab is disabled
  {
    AssignmentStatus status = lab.ForceActivateExperiment("report_test",
                                                          "user_id_8",
                                                          "treatment");
    EXPECT_EQ(AssignmentStatus::Unassigned, status);

    // Verify that experiment name and variation are evaluated even if lab is disabled when attempting a force
    status = lab.ForceActivateExperiment("invalid",
                                         "user_id_8",
                                         "treatment");
    EXPECT_EQ(AssignmentStatus::ExperimentNotFound, status);
    status = lab.ForceActivateExperiment("report_test",
                                         "user_id_8",
                                         "invalid");
    EXPECT_EQ(AssignmentStatus::VariantNotFound, status);
  }

  // Verify that lab can be re-enabled and experiment assigned
  lab.Enable(true);
  EXPECT_TRUE(lab.IsEnabled());
  {
    std::string variationKey;
    AssignmentStatus status = lab.ActivateExperiment("report_test",
                                                     "user_id_8",
                                                     {},
                                                     variationKey);
    EXPECT_EQ(AssignmentStatus::Assigned, status);
    EXPECT_EQ("treatment", variationKey);
    EXPECT_TRUE(lab.IsActiveExperiment("report_test", "user_id_8"));
  }

  // Verify that lab can be subsequently disabled and experiments are unassigned
  lab.Enable(false);
  EXPECT_FALSE(lab.IsEnabled());
  EXPECT_FALSE(lab.IsActiveExperiment("report_test", "user_id_8"));

}

TEST_F(AnkiLabTest, HandleActiveAssignmentUpdate)
{
  size_t assignmentsSize = 0;

  AnkiLab lab = CreateAnkiLabAB50();
  EXPECT_TRUE(lab.IsEnabled());

  Signal::SmartHandle handle = lab.ActiveAssignmentsUpdatedSignal().ScopedSubscribe(
    [&assignmentsSize](const std::vector<AssignmentDef>& activeAssignments) {
      assignmentsSize = activeAssignments.size();
    });

  std::string variationKey;
  AssignmentStatus status = lab.ActivateExperiment("report_test_all_treatment",
                                                   "user_id_8",
                                                   {},
                                                   variationKey);
  EXPECT_EQ(AssignmentStatus::Assigned, status);
  EXPECT_EQ("treatment", variationKey);
  EXPECT_EQ(1, assignmentsSize);
}

TEST_F(AnkiLabTest, RestorePreviousAssignments)
{
  AnkiLab lab = CreateAnkiLabAB50();
  EXPECT_TRUE(lab.IsEnabled());

  // Simulate previously being in the "control" group for an experiment that changed
  // and put 100% of users in the "treatment" group.  Our user should remain in "control"
  AssignmentStatus status =
    lab.RestoreActiveExperiment("report_test_all_treatment", "user_id_8", "control");
  EXPECT_EQ(AssignmentStatus::Assigned, status);

  std::string variationKey;
  status = lab.ActivateExperiment("report_test_all_treatment",
                                  "user_id_8",
                                  {},
                                  variationKey);
  EXPECT_EQ(AssignmentStatus::Assigned, status);
  EXPECT_EQ("control", variationKey);

  // Test trying to restore an experiment that is no longer running
  uint32_t epochSec = Util::EpochSecFromIso8601UTCDateString("2017-05-01T00:00:00Z");
  status = lab.RestoreActiveExperiment("report_test_times", "user_id_8", "treatment", epochSec);
  EXPECT_EQ(AssignmentStatus::ExperimentNotRunning, status);
}

TEST_F(AnkiLabTest, RestoreAssignmentWithAudienceFiltering)
{
  AnkiLab lab = CreateAnkiLabAB50();

  // First try to assign experiment with audience mismatch
  std::string variationKey;
  std::string experimentKey = "report_test_first_time_user";
  std::string userId = "user_id_0";
  std::vector<std::string> audienceTags = {};
  AssignmentStatus status;
  status = lab.ActivateExperiment(experimentKey,
                                  userId,
                                  audienceTags,
                                  variationKey);
  EXPECT_EQ(AssignmentStatus::AudienceMismatch, status);
  EXPECT_EQ("", variationKey);

  // Now restore an assignment, even though we are NOT in the audience
  status = lab.RestoreActiveExperiment(experimentKey, userId, "treatment");

  // A subsequent call to activate the experiment should succeed, even if the audience
  // does not match
  status = lab.ActivateExperiment(experimentKey,
                                  userId,
                                  audienceTags,
                                  variationKey);
  EXPECT_EQ(AssignmentStatus::Assigned, status);
  EXPECT_EQ("treatment", variationKey);

}


} // namespace AnkiLab
} // namespace DriveEngine
} // namespace Anki
