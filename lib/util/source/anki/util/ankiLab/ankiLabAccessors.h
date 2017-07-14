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

#ifndef __util_ankiLab_ankiLabAccessors_H_
#define __util_ankiLab_ankiLabAccessors_H_

#include "util/ankiLab/ankiLabDef.h"

#include <string>
#include <vector>

namespace Anki {
namespace Util {
namespace AnkiLab {

using AudienceTagList = std::vector<std::string>;

//
// AnkiLabDef accessors
//

// @return Experiment exists in lab definition with the specified `experimentKey`.
// @return nullptr if experiment is not found.
const Experiment* FindExperiment(const AnkiLabDef* lab,
                                 const std::string& experimentKey);

// @return true if timestamp falls within experiment date range
bool IsExperimentRunning(const Experiment* experiment,
                         const uint32_t timeSince1970Sec);

// @return true if audience tags intersect completely with audience
// tags required to participate in experiment.
bool IsMatchingAudience(const Experiment* experiment,
                        const AudienceTagList& audienceTags);


// @return ExperimentVariation with the specified `variationKey` in `experiment`.
// @return nullptr if no variant is not found
const ExperimentVariation* FindVariation(const Experiment* experiment,
                                         const std::string& variationKey);

// Determine whether `userHashBucket` value falls into experimental bucket range.
// @return ExperimentVariation if user is part of the experiment.
// @return nullptr if the user is not part of the experiment.
const ExperimentVariation* GetExperimentVariation(const Experiment* experiment,
                                                  const uint8_t userHashBucket);

//
// Utility Helpers
//

// @return value 0-99 representing a "bucket" value for experiment-user pair
// for the purpose of assigning a user to an experimental group (variation).
uint8_t CalculateExperimentHashBucket(const std::string& experimentKey,
                                      const std::string& userId);

// @return true if testSet is a subset of targetSet, otherwise returns false.
bool AudienceListIsSubsetOfList(const AudienceTagList& testSet, const AudienceTagList& targetSet);

AudienceTagList GetKnownAudienceTags(const AnkiLabDef& lab);

} // namespace AnkiLab
} // namespace Util
} // namespace Anki

#endif // __util_ankiLab_ankiLabAccessors_H_
