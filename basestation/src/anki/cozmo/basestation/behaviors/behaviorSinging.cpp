/**
 * File: BehaviorSinging.cpp
 *
 * Author: Al Chaussee
 * Created: 4/25/17
 *
 * Description: Behavior for Cozmo Sings
 *              Plays GetIn, Song, and GetOut animation sequence
 *              While running, cubes can be shaken to modify audio parameters for the song
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorSinging.h"

#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/cubeAccelComponent.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
  static const char* kAudioSwitchGroup = "audioSwitchGroup";
  static const char* kAudioSwitch = "audioSwitch";
  
  static const AnimationTrigger kGetInTrigger = AnimationTrigger::Singing_GetIn;
  static const AnimationTrigger kGetOutTrigger = AnimationTrigger::Singing_GetOut;
  
  static const AnimationTrigger k80BpmTrigger = AnimationTrigger::Singing_80bpm;
  static const AnimationTrigger k100BpmTrigger = AnimationTrigger::Singing_100bpm;
  static const AnimationTrigger k120BpmTrigger = AnimationTrigger::Singing_120bpm;
  
  static const AudioMetaData::GameParameter::ParameterType kVibratoParam =
    AudioMetaData::GameParameter::ParameterType::Cozmo_Singing_Vibrato;
  
  constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersSinging = {
    {ReactionTrigger::CliffDetected,                false},
    {ReactionTrigger::CubeMoved,                    true},
    {ReactionTrigger::DoubleTapDetected,            true},
    {ReactionTrigger::FacePositionUpdated,          true},
    {ReactionTrigger::FistBump,                     false},
    {ReactionTrigger::Frustration,                  false},
    {ReactionTrigger::Hiccup,                       false},
    {ReactionTrigger::MotorCalibration,             false},
    {ReactionTrigger::NoPreDockPoses,               false},
    {ReactionTrigger::ObjectPositionUpdated,        true},
    {ReactionTrigger::PlacedOnCharger,              false},
    {ReactionTrigger::PetInitialDetection,          true},
    {ReactionTrigger::PyramidInitialDetection,      true},
    {ReactionTrigger::RobotPickedUp,                false},
    {ReactionTrigger::RobotPlacedOnSlope,           false},
    {ReactionTrigger::ReturnedToTreads,             false},
    {ReactionTrigger::RobotOnBack,                  false},
    {ReactionTrigger::RobotOnFace,                  false},
    {ReactionTrigger::RobotOnSide,                  false},
    {ReactionTrigger::RobotShaken,                  false},
    {ReactionTrigger::Sparked,                      false},
    {ReactionTrigger::StackOfCubesInitialDetection, true},
    {ReactionTrigger::UnexpectedMovement,           true},
    {ReactionTrigger::VC,                           false}
  };
  
  static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersSinging),
                "Reaction triggers duplicate or non-sequential");
}

BehaviorSinging::BehaviorSinging(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  std::string value;
  // Assert that displayNameKey exists in the config. All the singing behaviors
  // need it to display in app text
  bool res = JsonTools::GetValueOptional(config, "displayNameKey", value);
  DEV_ASSERT_MSG(res,
                 "BehaviorSinging.DisplayNameKeyRequired",
                 "No displayName for singing behavior %s",
                 GetName().c_str());

  // We have to have a audioSwitchGroup otherwise this behavior is useless
  res = JsonTools::GetValueOptional(config, kAudioSwitchGroup, value);
  DEV_ASSERT_MSG(res,
                 "BehaviorSinging.NoAudioSwitchGroup",
                 "No audioSwitchGroup for singing behavior %s",
                 GetName().c_str());
  _audioSwitchGroup = AudioMetaData::SwitchState::SwitchGroupTypeFromString(value);
  
  // We have to have a audioSwitch otherwise this behavior is useless
  res = JsonTools::GetValueOptional(config, kAudioSwitch, value);
  DEV_ASSERT_MSG(res,
                 "BehaviorSinging.NoAudioSwitch",
                 "No audioSwitch for singing behavior %s",
                 GetName().c_str());

  // Depending on which SwitchGroup we are using, figure out which
  // specific switch and animation to use
  using GenericSwitch = AudioMetaData::SwitchState::GenericSwitch;
  switch(_audioSwitchGroup)
  {
    case AudioSwitchGroup::Cozmo_Sings_80Bpm:
    {
      _audioSwitch = static_cast<GenericSwitch>(AudioMetaData::SwitchState::Cozmo_Sings_80BpmFromString(value));
      _songAnimTrigger = k80BpmTrigger;
      break;
    }
    case AudioSwitchGroup::Cozmo_Sings_100Bpm:
    {
      _audioSwitch = static_cast<GenericSwitch>(AudioMetaData::SwitchState::Cozmo_Sings_100BpmFromString(value));
      _songAnimTrigger = k100BpmTrigger;
      break;
    }
    case AudioSwitchGroup::Cozmo_Sings_120Bpm:
    {
      _audioSwitch = static_cast<GenericSwitch>(AudioMetaData::SwitchState::Cozmo_Sings_120BpmFromString(value));
      _songAnimTrigger = k120BpmTrigger;
      break;
    }
    default:
    {
      DEV_ASSERT_MSG(false,
                     "BehaviorSinging.NoAudioSwitch",
                     "No audioSwitch for singing behavior %s",
                     GetName().c_str());
      
      // Default to a 80Bpm fallback song
      _audioSwitch = GenericSwitch::Invalid;
      _audioSwitchGroup = AudioSwitchGroup::Cozmo_Sings_80Bpm;
      _songAnimTrigger = k80BpmTrigger;
    }
  }
}

BehaviorSinging::~BehaviorSinging()
{
  
}

bool BehaviorSinging::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  // Always runnable, the higher level Singing goal/activity is responsible
  // for deciding when Cozmo should sing
  return true;
}

Result BehaviorSinging::InitInternal(Robot& robot)
{
  // Tell Wwise to switch to the specific SwitchGroup (80/100/120bpm)
  // and a specific switch in the group (song)
  robot.GetRobotAudioClient()->PostRobotSwitchState(_audioSwitchGroup,
                                                    _audioSwitch);

  // Disable reactions
  SmartDisableReactionsWithLock(GetName(), kAffectTriggersSinging);

  // Clear listeners and averages
  _cubeAccelListeners.clear();
  _objectShakeAverages.clear();
  
  // Filter to find all LightCubes
  BlockWorldFilter filter;
  filter.AddAllowedFamily(ObjectFamily::LightCube);
  filter.SetFilterFcn(nullptr);
  
  // Get all connected light cubes
  std::vector<const ActiveObject*> connectedObjects;
  robot.GetBlockWorld().FindConnectedActiveMatchingObjects(filter, connectedObjects);
  
  // For each of the connected light cubes
  for(const ActiveObject* object : connectedObjects)
  {
    const ObjectID& objectID = object->GetID();
    
    // Create a RollingAverage to keep track of the average shake amount of this object
    _objectShakeAverages[objectID].Reset();
    
    auto shakeDetected = [this, &objectID](const float shakeAmount) {
      _objectShakeAverages[objectID].Update(shakeAmount);
    };
    
    // Set up a CubeAccel ShakeListener that will update this object's average shake amount
    // when shaking is detected
    std::shared_ptr<CubeAccelListeners::ICubeAccelListener> listener;
    listener.reset(new CubeAccelListeners::ShakeListener(0.5, 2.5, 3.9, shakeDetected));
    robot.GetCubeAccelComponent().AddListener(objectID, listener);
    
    // Store the listener so we can remove it when the behavior ends
    // TODO Add SmartAddCubeAccelListener/SmartRemoveCubeAccelListener to base class
    _cubeAccelListeners.emplace_back(objectID, listener);
  }
  
  // Setup the only action this behavior does, three sequential animations
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  action->AddAction(new TriggerAnimationAction(robot, kGetInTrigger));
  action->AddAction(new TriggerAnimationAction(robot, _songAnimTrigger));
  action->AddAction(new TriggerAnimationAction(robot, kGetOutTrigger));
  StartActing(action, [this](const ActionResult& res) {
    if(res == ActionResult::SUCCESS)
    {
      BehaviorObjectiveAchieved(BehaviorObjective::Singing);
    }
  });
  
  return RESULT_OK;
}

BehaviorStatus BehaviorSinging::UpdateInternal(Robot& robot)
{
  // Figure out which object was, on average, shaken the most this tick
  float mostShakenObjectAverage = 0;
  for(auto& objectShakeAverage : _objectShakeAverages)
  {
    if(objectShakeAverage.second.Avg() > mostShakenObjectAverage)
    {
      mostShakenObjectAverage = objectShakeAverage.second.Avg();
    }
    
    objectShakeAverage.second.Reset();
  }
  
  // Based on the parameters of the ShakeListener, this maxShakeAmount is
  // the shake amount that results from fairly aggressive shaking
  // The shake amount at which vibrato will be max
  static const float kMaxShakeAmount = 3000.f;
  const float vibratoScale = CLIP(mostShakenObjectAverage/kMaxShakeAmount, 0.f, 1.f);

  // Low pass filter on vibrato scale to smooth it out
  static const float kVibratoLowPassCoeff = 0.5f;
  _vibratoScaleFilt = vibratoScale * kVibratoLowPassCoeff +
                      _vibratoScaleFilt * (1.f-kVibratoLowPassCoeff);

  // Post the filtered vibrato scale to wwise
  robot.GetRobotAudioClient()->PostRobotParameter(kVibratoParam,
                                                  _vibratoScaleFilt);
  
  return (IsActing() ? BehaviorStatus::Running : BehaviorStatus::Complete);
}

void BehaviorSinging::StopInternal(Robot& robot)
{
  robot.GetRobotAudioClient()->PostRobotParameter(kVibratoParam, 0);

  // Remove all our listeners
  for(auto iter = _cubeAccelListeners.begin(); iter != _cubeAccelListeners.end();)
  {
    const bool wasRemoved = robot.GetCubeAccelComponent().RemoveListener(iter->first, iter->second);
    DEV_ASSERT(wasRemoved, "BehaviorSinging.StopInternal.RemoveListenerFailed");
    
    // We should be the only thing that has a shared_ptr pointing to our CubeAccelListeners
    // CubeAccelComponent used to have one but we just removed it
    DEV_ASSERT(iter->second.use_count() == 1, "BehaviorSinging.StopInternal.ListenerHasMultipleRefs");
    iter = _cubeAccelListeners.erase(iter);
  }
}
  
}
}
