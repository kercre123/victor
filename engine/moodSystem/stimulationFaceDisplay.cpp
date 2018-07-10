/**
 * File: stimulationFaceDisplay.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-03-28
 *
 * Description: Component which uses the "Stimulated" mood to modify the robot's face (current design is to
 *              modify saturation level)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/moodSystem/stimulationFaceDisplay.h"

#include "clad/types/featureGateTypes.h"
#include "engine/components/animationComponent.h"
#include "engine/contextWrapper.h"
#include "engine/cozmoContext.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/utils/cozmoFeatureGate.h"
#include "lib/util/source/anki/util/entityComponent/dependencyManagedEntity.h"
#include "lib/util/source/anki/util/graphEvaluator/graphEvaluator2d.h"
#include "util/console/consoleInterface.h"
#include "util/math/math.h"

namespace Anki {
namespace Cozmo {

namespace {

CONSOLE_VAR(bool, kStimFace_enabled, "StimFace", true);
CONSOLE_VAR_RANGED(int, kStimFace_ema_N, "StimFace", 20, 0, 100);
CONSOLE_VAR_RANGED(f32, kStimFace_sendThresh, "StimFace", 0.01f, 0.0f, 1.0f );
CONSOLE_VAR_RANGED(f32, kStimFace_minSaturation, "StimFace", 0.25f, 0.0f, 1.0f );

}

StimulationFaceDisplay::StimulationFaceDisplay()
  : IDependencyManagedComponent(this, RobotComponentID::StimulationFaceDisplay)
{
}

StimulationFaceDisplay::~StimulationFaceDisplay()
{
}

void StimulationFaceDisplay::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps)
{
  _saturationMap = std::make_unique<Util::GraphEvaluator2d>();

  // for now just use a direct mapping
  _saturationMap->AddNode(0.0f, kStimFace_minSaturation);
  _saturationMap->AddNode(1.0f, 1.0f);
}


void StimulationFaceDisplay::UpdateDependent(const RobotCompMap& dependentComps)
{
  if( !kStimFace_enabled ) {
    return;
  }

  // disable stimulation-dependent face level for PR demo
  auto* context = dependentComps.GetComponent<ContextWrapper>().context;
  const bool inPRDemo = context->GetFeatureGate()->IsFeatureEnabled( FeatureType::PRDemo );
  if( inPRDemo ) {
    return;
  }
             
  auto& moodManager = dependentComps.GetComponent<MoodManager>();
  const float stim = Anki::Util::Clamp(moodManager.GetEmotionValue(EmotionType::Stimulated), 0.0f, 1.0f);

  const float alpha = 2.0f/(1.0f+kStimFace_ema_N);
  // update exponential moving average
  _ema = stim * alpha + _ema * (1 - alpha);

  float sat = _ema;
  if( _saturationMap ) {
    sat = _saturationMap->EvaluateY(_ema);
  }
  
  const float diff = std::fabs(sat - _lastSentSatLevel);
  if( _lastSentSatLevel < 0.0f || diff > kStimFace_sendThresh ) {
    auto& animComponent = dependentComps.GetComponent<AnimationComponent>();
    animComponent.SetFaceSaturation( sat );
    _lastSentSatLevel = sat;
  }
}

}
}
