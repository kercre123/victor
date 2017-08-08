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

#ifndef ANKI_COZMO_BASESTATION_COZMO_EXPERIMENTS_H
#define ANKI_COZMO_BASESTATION_COZMO_EXPERIMENTS_H

#include "engine/utils/cozmoAudienceTags.h"
#include "util/ankiLab/ankiLab.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

class CozmoContext;

class CozmoExperiments
{
public:
  CozmoExperiments(const CozmoContext* context);

  CozmoAudienceTags& GetAudienceTags() { return _tags; }
  const CozmoAudienceTags& GetAudienceTags() const { return _tags; }

  Util::AnkiLab::AnkiLab& GetAnkiLab() { return _lab; }
  const Util::AnkiLab::AnkiLab& GetAnkiLab() const { return _lab; }

  void InitExperiments();
  void AutoActivateExperiments(const std::string& userId);

  Util::AnkiLab::AssignmentStatus ActivateExperiment(const Util::AnkiLab::ActivateExperimentRequest& request,
                                                     std::string& outVariationKey);

private:
  const CozmoContext* _context;
  Util::AnkiLab::AnkiLab _lab;
  CozmoAudienceTags _tags;
};

}
}

#endif
