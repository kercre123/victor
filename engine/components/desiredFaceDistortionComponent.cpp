/**
 * File: desiredFaceDistortionComponent.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-06-21
 *
 * Description: A simple component that uses the robots needs level to compute if / how much the robot's face
 *              (eyes) should be procedurally distorted
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/desiredFaceDistortionComponent.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Vector {

namespace {
// below this amount, don't bother to distort at all. NOTE: may go lower than this slightly with randomness
static constexpr float kMinDesiredDistortionDegree = 0.1f;
}

DesiredFaceDistortionComponent::DesiredFaceDistortionComponent()
{
}

void DesiredFaceDistortionComponent::Init(const Json::Value& config, Util::RandomGenerator* rng)
{
  _params = std::make_unique<Params>(config["needsBasedFaceDistortion"]);
  _rng = rng;
}

DesiredFaceDistortionComponent::Params::Params(const Json::Value& config)
  : _cooldownEvaluator( new Util::GraphEvaluator2d )
  , _degreeEvaluator( new Util::GraphEvaluator2d )
{

  {
    const Json::Value& cooldownEvaluatorConfig = config["cooldown"];
    if( ANKI_VERIFY( !cooldownEvaluatorConfig.isNull(),
                     "DesiredFaceDistortionComponent.ConfigError.NoCooldownConfig",
                     "No cooldown config specified")) {
      const bool result = _cooldownEvaluator->ReadFromJson(cooldownEvaluatorConfig);

      if( !result || _cooldownEvaluator->GetNumNodes() == 0 ) {
        PRINT_NAMED_ERROR("DesiredFaceDistortionComponent.ConfigError.CooldownParsingFailed",
                          "failed to parse cooldown graph evaluator");
      }
    }
  }

  _cooldownRangeMultiplier = config.get("cooldown_range_multiplier", _cooldownRangeMultiplier).asFloat();

  
  {
    const Json::Value& degreeEvaluatorConfig = config["degree"];
    if( ANKI_VERIFY( !degreeEvaluatorConfig.isNull(),
                     "DesiredFaceDistortionComponent.ConfigError.NoDegreeConfig",
                     "No degree config specified")) {
      const bool result = _degreeEvaluator->ReadFromJson(degreeEvaluatorConfig);

      if( !result || _cooldownEvaluator->GetNumNodes() == 0 ) {
        PRINT_NAMED_ERROR("DesiredFaceDistortionComponent.ConfigError.DegreeParsingFailed",
                          "failed to parse degree graph evaluator");
      }
    }
  }

  _degreeRangeMultiplier = config.get("degree_range_multiplier", _degreeRangeMultiplier).asFloat();
}

DesiredFaceDistortionComponent::Params::~Params()
{
}

float DesiredFaceDistortionComponent::GetCurrentDesiredDistortion()
{
  // no repair-based eye distortion on Victor. Leave this code here because it may be used in another way
  return -1.0f;

  
  // Return the same distortion value for the duration of the tick
  const size_t tickCount = BaseStationTimer::getInstance()->GetTickCount();
  if(tickCount == _prevTickCount)
  {
    return _curDistortion;
  }
  
  _curDistortion = -1.f;
  _prevTickCount = tickCount;
  
  if( _params != nullptr && _rng != nullptr) {

    // if it's time to distort again, or we've never distorted, calculate a desired distortion
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
    const bool neverDistorted = _nextTimeToDistort_s < 0.0f;
    const bool timeToDistort = neverDistorted || _nextTimeToDistort_s <= currTime_s;

    if( timeToDistort ) {

      const float repairLevel = 1.0f;

      // evaluate what the "degree" would be at this level
      const float desiredDegree = _params->_degreeEvaluator->EvaluateY( repairLevel );

      if( desiredDegree >= kMinDesiredDistortionDegree ) {
        // a distortion is desired, let's do it. First, define the actual distortion amount with randomization

        const float degree_min = desiredDegree * ( 1.0f - _params->_degreeRangeMultiplier / 2 );
        const float degree_max = desiredDegree * ( 1.0f + _params->_degreeRangeMultiplier / 2 );

        const float degree = (float) _rng->RandDblInRange(degree_min, degree_max);

        // now compute the next cooldown
        const float desiredCooldownTime_s = _params->_cooldownEvaluator->EvaluateY( repairLevel );
    
        const float cooldownTime_min_s = desiredCooldownTime_s * ( 1.0f - _params->_cooldownRangeMultiplier / 2 );
        const float cooldownTime_max_s = desiredCooldownTime_s * ( 1.0f + _params->_cooldownRangeMultiplier / 2 );
    
        const float cooldownTime_s = (float) _rng->RandDblInRange(cooldownTime_min_s, cooldownTime_max_s);

        PRINT_CH_DEBUG("NeedsSystem", "DesiredFaceDistortionComponent.DistortingFace.Degree",
                       "Repair level: %f, degree: %f (desired %f, range(%f-%f))",
                       repairLevel,
                       degree,
                       desiredDegree,
                       degree_min,
                       degree_max);

        PRINT_CH_DEBUG("NeedsSystem", "DesiredFaceDistortionComponent.DistortingFace.Cooldown",
                       "Repair level: %f, cooldown: %fs (desired %f, range(%f-%f))",
                       repairLevel,
                       cooldownTime_s,
                       desiredCooldownTime_s,
                       cooldownTime_min_s,
                       cooldownTime_max_s);

        _nextTimeToDistort_s = currTime_s + cooldownTime_s;
        _curDistortion = degree;
        return _curDistortion;
      }
    }
    
  } // if params exists
  
  // don't distort
  return -1.0f;
}


}
}
