/**
 * File: ankiLab.h
 *
 * Author: chapados
 * Created: 1/25/17
 *
 * Collection of functions to access derived data from AnkiLabDef (POD objects) in
 * a purely function way. Each function represents a lookup or calculation that can
 * be performed on a raw data object. Rather than only hide these inside a class, they
 * are public functions to facilitate testing, validation or building alternative interfaces
 * to the underlying data.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "util/ankiLab/ankiLabAccessors.h"

#include "util/crc/crc.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/string/stringUtils.h"

#include <ctime>
#include <cmath>

#define LOG_CHANNEL    "AnkiLab"

namespace Anki {
namespace Util {
namespace AnkiLab {

const Experiment* FindExperiment(const AnkiLabDef* lab,
                                 const std::string& experimentKey)
{
  ASSERT_NAMED(nullptr != lab, "AnkiLab must not be NULL");

  // Find experiment
  const std::vector<Experiment>& experiments = lab->GetExperiments();
  auto search = std::find_if(experiments.begin(), experiments.end(),
                                    [&experimentKey](const Experiment& e) {
                                      return (e.GetKey() == experimentKey);
                                    });
  if (search == experiments.end()) {
    // experimentKey not found
    return nullptr;
  } else {
    return &(*search);
  }
}

bool IsExperimentRunning(const Experiment* experiment,
                         const uint32_t timeSince1970Sec)
{
  ASSERT_NAMED(nullptr != experiment, "Experiment pointer must not be NULL");

  uint32_t now = timeSince1970Sec;

  uint32_t startTimeSec = Util::EpochSecFromIso8601UTCDateString(experiment->GetStart_time_utc_iso8601());

  if (now < startTimeSec) {
    // experiment not started
    return false;
  }

  uint32_t stopTimeSec = Util::EpochSecFromIso8601UTCDateString(experiment->GetStop_time_utc_iso8601());
  if ((stopTimeSec > 0) && (now >= stopTimeSec)) {
    // experiment stopped
    return false;
  }

  uint32_t pauseTimeSec = Util::EpochSecFromIso8601UTCDateString(experiment->GetPause_time_utc_iso8601());
  uint32_t resumeTimeSec = Util::EpochSecFromIso8601UTCDateString(experiment->GetResume_time_utc_iso8601());
  if (   ((pauseTimeSec > 0) && (now >= pauseTimeSec))
      && ((now < resumeTimeSec) || (resumeTimeSec == 0))) {
    // experiment paused
    return false;
  }

  return true;
}

// returns true if audience tags intersect completely with audience
// tags required to participate in experiment.
bool IsMatchingAudience(const Experiment* experiment,
                        const AudienceTagList& audienceTags)
{
  ASSERT_NAMED(nullptr != experiment, "Experiment pointer must not be NULL");

  const std::vector<std::string>& experimentalAudience = experiment->GetAudience_tags();
  if (experimentalAudience.empty()) {
    return true;
  } else {
    return AudienceListIsSubsetOfList(experimentalAudience, audienceTags);
  }
}

const ExperimentVariation* FindVariation(const Experiment* experiment,
                                         const std::string& variationKey)
{
  ASSERT_NAMED(nullptr != experiment, "Experiment pointer must not be NULL");

  const std::vector<ExperimentVariation>& variations = experiment->GetVariations();
  const auto& search =
    std::find_if(variations.begin(), variations.end(),
                 [&variationKey](const ExperimentVariation& v) {
                   return (v.GetKey() == variationKey);
                 });

  if (search == variations.end()) {
    return nullptr;
  } else {
    return &(*search);
  }
}

const ExperimentVariation* GetExperimentVariation(const Experiment* experiment,
                                                  const uint8_t userHashBucket)
{
  ASSERT_NAMED(nullptr != experiment, "Experiment pointer must not be NULL");

  LOG_DEBUG("AnkiLab.GetExperimentVariation.bucket",
            "%s : %u",
            experiment->GetKey().c_str(), userHashBucket);

  // Calculate population fraction of each variation
  const std::vector<ExperimentVariation>& variations = experiment->GetVariations();

  bool inExperiment = false;
  const ExperimentVariation* variation = nullptr;
  uint8_t bucketStart = 0;
  for (const ExperimentVariation& v : variations) {
    uint8_t maxBucketSize = v.GetPop_frac_pct();
    if (maxBucketSize > 0) {
#if ALLOW_DEBUG_LOGGING
      uint8_t maxBucketEnd = bucketStart + (maxBucketSize - 1);
#endif
      uint8_t bucketSize = (experiment->GetPop_frac_pct() * v.GetPop_frac_pct()) / 100.f;
      if (bucketSize > 0) {
        uint8_t bucketEnd = bucketStart + bucketSize - 1;

        LOG_DEBUG("AnkiLab.GetExperimentVariation.bucket.range",
                  "|[%2u - %2u] - %2u| : %s",
                  bucketStart, bucketEnd, maxBucketEnd,
                  v.GetKey().c_str());

        if ((userHashBucket >= bucketStart) && (userHashBucket <= bucketEnd)) {
          variation = &v;
          inExperiment = true;
          break;
        }
      }

      bucketStart += maxBucketSize;
    }
  }

  if (!inExperiment) {
    return nullptr;
  } else {
    LOG_DEBUG("AnkiLab.GetExperimentVariation.bucket.assign",
              "%s : %u : %s",
              experiment->GetKey().c_str(),
              userHashBucket,
              variation->GetKey().c_str());
    return variation;
  }
}

uint8_t CalculateExperimentHashBucket(const std::string& experimentKey,
                                      const std::string& userId)
{
  ASSERT_NAMED(!experimentKey.empty(),
               "CalculateExperimentHashBucket: experimentKey can not be empty");
  ASSERT_NAMED(!userId.empty(),
               "CalculateExperimentHashBucket: userId can not be empty");

  uint16_t crc = 0xffff;
  const char* expStr = experimentKey.c_str();
  for (ssize_t i = 0; i < experimentKey.length(); ++i) {
    crc = update_crc_ccitt(crc, expStr[i]);
  }
  const char* userIdStr = userId.c_str();
  for (ssize_t i = 0; i < userId.length(); ++i) {
    crc = update_crc_ccitt(crc, userIdStr[i]);
  }

  // assume crc value distributes string bytes evenly across uint16_t range
  uint32_t percentBucket = static_cast<uint32_t>(floorf(crc / 655.36f));
  uint8_t hash = Anki::Util::numeric_cast_clamped<uint8_t>(percentBucket);
  return hash;
}

bool AudienceListIsSubsetOfList(const AudienceTagList& testSet,
                                const AudienceTagList& targetSet)
{
  // copy & sort id sets
  AudienceTagList testSetSorted = testSet;
  std::sort(testSetSorted.begin(), testSetSorted.end());

  AudienceTagList targetSetSorted = targetSet;
  std::sort(targetSetSorted.begin(), targetSetSorted.end());

  // intersect sets: this will produce a list of strings in both vectors
  AudienceTagList intersectionList;
  std::set_intersection(testSetSorted.begin(), testSetSorted.end(),
                        targetSetSorted.begin(), targetSetSorted.end(),
                        std::back_inserter(intersectionList));

  // if intersection list is the same as the sorted test list, then testSet is a subset of targetSet
  const bool isSubset = (intersectionList == testSetSorted);

  return isSubset;
}

AudienceTagList GetKnownAudienceTags(const AnkiLabDef& labDef)
{
  AudienceTagList tags;
  for (const auto& experiment : labDef.experiments) {
    for (const auto& tag : experiment.audience_tags) {
      tags.push_back(tag);
    }
  }
  return tags;
}

} // namespace AnkiLab
} // namespace Util
} // namespace Anki
