/**
 * File: desiredFaceDistortionComponent.h
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

#ifndef __Cozmo_Basestation_Components_DesiredFaceDistortionComponent_H__
#define __Cozmo_Basestation_Components_DesiredFaceDistortionComponent_H__

#include "json/json-forwards.h"
#include "util/helpers/noncopyable.h"

#include <memory>

namespace Anki {

namespace Util {
class GraphEvaluator2d;
class RandomGenerator;
}

namespace Cozmo {


class DesiredFaceDistortionComponent : private Anki::Util::noncopyable
{
public:

  DesiredFaceDistortionComponent();

  void Init(const Json::Value& config, Util::RandomGenerator* rng);

  // If we should start a new face distortion _now_, this returns the degree to which the face should be
  // distorted (see scanlineDistorter for more info), otherwise it returns a negative number. Once it returns
  // a positive number, it will return negative until it's time to start a _new_ face distortion
  float GetCurrentDesiredDistortion();

private:

  struct Params : private Anki::Util::noncopyable {
    Params(const Json::Value& config);
    ~Params();
    
    // x = repair need level
    // y = average cooldown until the next distortion event
    std::unique_ptr<Util::GraphEvaluator2d> _cooldownEvaluator;

    // how "random" the cooldown should be. Current cooldown value from the evaluator will be multiplied by this
    // factor to get the maximum range of the uniform random cooldown time
    float _cooldownRangeMultiplier = 0.0f;

    // x = repair need level
    // y = "degree" of distortion (roughly correlated to pixels)
    std::unique_ptr<Util::GraphEvaluator2d> _degreeEvaluator;

    // As with cooldown, this is the factor determining the amount of randomness on the degree
    float _degreeRangeMultiplier = 0.0f;
  };

  std::unique_ptr<const Params> _params;

  float _nextTimeToDistort_s = -1.0f;
  
  Util::RandomGenerator* _rng = nullptr;
  
  float  _curDistortion = -1.f;
  size_t _prevTickCount = 0;
};


}
}

#endif
