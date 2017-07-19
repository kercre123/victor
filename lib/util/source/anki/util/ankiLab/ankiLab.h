/**
 * File: ankiLab.h
 *
 * Author: chapados
 * Created: 1/25/17
 *
 * Description: Interface to A/B test data for experimental features.
 * Note that this class only provides an interface to data and operations related
 * directly to experiment data.  It has no knowledge of user profiles or outside
 * configuration data.  It provides a simple interface to determine whether an
 * experiment is valid / running and for reproducibly assigning an experiment
 * variation for a user_id + experiment_id combination.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __util_ankiLab_ankiLab_H_
#define __util_ankiLab_ankiLab_H_

#include <set>
#include <string>
#include <vector>

#include "util/ankiLab/ankiLabDef.h"
#include "util/signals/simpleSignal.hpp"

namespace Anki {
namespace Util {
namespace AnkiLab {

class IAnkiLabDataStore;

class AnkiLab
{
public:

  AnkiLab();

  // Enables/disables the testing lab
  void Enable(const bool enabled);

  // Accessor for whether or not the lab is enabled
  bool IsEnabled() const { return _enabled; }

  // Set function user to convert an (experimentKey, userId) tuple to an 'bucket'
  // value from 0-99, used to determine the group to which the user will be assigned.
  using ExperimentUserHashFunction =
    std::function<uint8_t (const std::string& experimentKey, const std::string& userId)>;
  void SetExperimentUserHashFunction(ExperimentUserHashFunction hashFunc);

  // Load Experiments from JSON
  //
  // @return true if data was loaded successfully, otherwise false.
  bool Load(const std::string& jsonContents);

  //
  // Access read-only data from AnkiLabDef
  //

  // Access underlying `AnkiLabDef` struct containing raw experiment definitions
  const AnkiLabDef& GetAnkiLabDefinition() const;

  // Find experiment with specified key.
  // @return Experiment pointer if experiment exists, otherwise nullptr.
  const Experiment* FindExperiment(const std::string& experimentKey) const;

  //
  // Queries
  //

  // @return true if user has been assigned a group variant in the specified experiment during
  // the current apprun.
  bool IsActiveExperiment(const std::string& experimentKey,
                          const std::string& userId) const;
  
  //
  // Actions
  //

  // Attempt to assign a user to an experiment group (variation).
  //
  // @return true if:
  //        experiment audience tags are a subset of audience tags
  //   AND  experiment is running
  //   AND  user is included in experiment
  //        (experiment+user hash falls into experimental bucket range)
  // If user is part of the experiment, the outVariationKey will contain the key
  // key for the variation that the user is bucketed in.
  // This method generates analytics (DAS) events based on the outcome of the
  // assignment attempt.
  //
  // @return AssignmentStatus to indicate invalid input, unassigned or assigned outcomes.
  AssignmentStatus ActivateExperiment(const std::string& experimentKey,
                                      const std::string& userId,
                                      const std::vector<std::string>& audienceTags,
                                      const uint32_t epochSec,
                                      std::string& outVariationKey);

  // Attempt to assign a user to an experiment group (variation).
  //
  // Identical to ActivateExperiment, except that the current time parameter
  // is automatically determined based on the system clock.
  AssignmentStatus ActivateExperiment(const std::string& experimentKey,
                                      const std::string& userId,
                                      const std::vector<std::string>& audienceTags,
                                      std::string& outVariationKey);

  AssignmentStatus RestoreActiveExperiment(const std::string& experimentKey,
                                           const std::string& userId,
                                           const std::string& variationKey) {
    return RestoreActiveExperiment(experimentKey, userId, variationKey, GetSecondsSinceEpoch());
  }

  AssignmentStatus RestoreActiveExperiment(const std::string& experimentKey,
                                           const std::string& userId,
                                           const std::string& variationKey,
                                           const uint32_t epochSec);

  // Activate experiment immediately with specified variation.
  //
  //
  // @return ForceAssigned if assignment was successful, otherwise one of
  // ExperimentNotFound or VariantNotFound if specified values are invalid.
  AssignmentStatus ForceActivateExperiment(const std::string& experimentKey,
                                           const std::string& userId,
                                           const std::string& variationKey);

  // Override variation assignment.
  // This overrides the default "assignment" step in the activation process,
  // causing the user to be assigned to the specified variation. All other
  // criteria must still be valid. Must be called before ActivateExperiment.
  //
  // @return true if group assignment will be overridden during activation, otherwise
  // false.
  bool OverrideExperimentVariation(const std::string& experimentKey,
                                   const std::string& userId,
                                   const std::string& variationKey);


  // Attempt to activate experiments defined as `automatic` with the specified `userId`
  // and `audienceTags`.
  // @return The number of experiments on which ActivateExperiment was called.
  size_t AutoActivateExperimentsForUser(const std::string& userId,
                                      const std::vector<std::string>& audienceTags);

  // Return a list of all audience tags in the experiments we know about
  std::vector<std::string> GetKnownAudienceTags() const;

  using ActiveAssignmentsUpdatedSignalType =
    Signal::Signal<void (const std::vector<AssignmentDef>& activeAssignments)>;

  ActiveAssignmentsUpdatedSignalType& ActiveAssignmentsUpdatedSignal() {
    return (*_activeAssignmentsUpdatedSignal);
  };

private:
  // Enabled by default in shipping builds and disabled for debug (aka developer) builds
  bool _enabled;

  // Hash function for calculating user bucket for experiment
  ExperimentUserHashFunction _expUserHashFunc;

  // AnkiLab data definitions
  AnkiLabDef _ankiLabDef;

  // keep track of "active" experiments
  std::vector<AssignmentDef> _activeAssignments;

  // keep track of "override variant" assignments
  std::vector<AssignmentDef> _overrideAssignments;

  // keep track of 'force' assignments
  std::vector<AssignmentDef> _forceAssignments;

  std::unique_ptr<ActiveAssignmentsUpdatedSignalType> _activeAssignmentsUpdatedSignal;

  // Helper method to find assignments based on `experimentKey` and `userId`.
  // @return pointer to `_activeAssignments` for (experimentKey, userId) tuple.
  AssignmentDef* FindAssignment(std::vector<AssignmentDef>& assignmentList,
                                const std::string& experimentKey,
                                const std::string& userId);

  // Add assignment to _activeAssignments
  void AssignExperimentVariation(const AssignmentDef& assignment);

  // Perform work required to assign a user to an experiment group (variation).
  //
  // This method performs that actual work of `ActivateExperiment` but does not report
  // any analytics events.
  //
  // @return AssignmentStatus to indicate invalid input, unassigned or assigned outcomes.
  AssignmentStatus ActivateExperimentInternal(const std::string& experimentKey,
                                              const std::string& userId,
                                              const std::vector<std::string>& audienceTags,
                                              const uint32_t epochSec,
                                              std::string& outVariationKey);

  // Generate and send analytics events for an experiment group assignment result.
  // This method generates:
  // event:experiment.assigned, event:experiment.unassigned, warning:experiment.invalid
  void ReportExperimentAssignmentResult(const AssignmentDef& assignment,
                                        const AssignmentStatus status);

  uint32_t GetSecondsSinceEpoch() const;
};

} // namespace AnkiLab
} // namespace Util
} // namespace Anki


#endif // __util_ankiLab_ankiLab_H_
