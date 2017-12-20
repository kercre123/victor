/**
 * File: behaviorRockPaperScissors.cpp
 *
 * Author: Andrew Stein
 * Created: 2017-12-18
 *
 * Description: R&D Behavior to play rock, paper, scissors.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorRockPaperScissors.h"

#include "clad/types/imageTypes.h"

#include "coretech/common/include/anki/common/basestation/jsonTools.h"
#include "coretech/common/include/anki/common/basestation/utils/data/dataPlatform.h"
#include "coretech/common/include/anki/common/basestation/utils/timer.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/bodyLightComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"

#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/quoteMacro.h"

namespace Anki {
namespace Cozmo {

namespace {

CONSOLE_VAR(bool, kRockPaperScissors_FakePowerButton, "RockPaperScissors", false);

static const BackpackLights kLightsOff = {
  .onColors               = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
  .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
  .onPeriod_ms            = {{0,0,0}},
  .offPeriod_ms           = {{0,0,0}},
  .transitionOnPeriod_ms  = {{0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0}},
  .offset                 = {{0,0,0}}
};

}

BehaviorRockPaperScissors::BehaviorRockPaperScissors(const Json::Value& config)
  : ICozmoBehavior(config)
{
  SubscribeToTags({ExternalInterface::MessageEngineToGameTag::RobotObservedGenericObject});
  
# define GET_FROM_CONFIG(_type_, _name_) \
  _##_name_ = JsonTools::Parse##_type_(config, QUOTE(_name_), "BehaviorRockPaperScissors")
  
  GET_FROM_CONFIG(Float,  waitAfterButton_sec);
  GET_FROM_CONFIG(Float,  waitTimeBetweenTaps_sec);
  GET_FROM_CONFIG(Float,  minDetectionScore);
  GET_FROM_CONFIG(Uint32, displayHoldTime_ms);
  
# undef GET_FROM_CONFIG
}

BehaviorRockPaperScissors::~BehaviorRockPaperScissors()
{
}

Result BehaviorRockPaperScissors::OnBehaviorActivated(BehaviorExternalInterface& bei)
{
  DEV_ASSERT(bei.HasAnimationComponent(), 
             "BehaviorRockPaperScissors.OnBehaviorActivated.NoAnimComponent");

  DEV_ASSERT(bei.HasVisionComponent(), 
             "BehaviorRockPaperScissors.OnBehaviorActivated.NoVisionComponent");

  _state = State::WaitForButton;
  _humanSelection = Selection::Unknown;
  _robotSelection = Selection::Unknown;
  
  return Result::RESULT_OK;
}

void BehaviorRockPaperScissors::OnBehaviorDeactivated(BehaviorExternalInterface& bei)
{        

}

BehaviorStatus BehaviorRockPaperScissors::UpdateInternal_WhileRunning(BehaviorExternalInterface& bei)
{
  switch(_state)
  {
    case State::WaitForButton:
    {
      if(bei.GetRobotInfo().IsPowerButtonPressed() || kRockPaperScissors_FakePowerButton || !bei.GetRobotInfo().IsPhysical())
      {
        kRockPaperScissors_FakePowerButton = false;
        _state = State::PlayCadence;
      }
      break;
    }

    case State::PlayCadence:
    {
      // Turn towards last face if we have one. Otherwise just look up.
      auto& faceWorld = bei.GetFaceWorld();
      IActionRunner* moveHeadAction = nullptr;
      if(faceWorld.HasAnyFaces()) {
        moveHeadAction = new TurnTowardsLastFacePoseAction();
      }
      else {
        moveHeadAction = new MoveHeadToAngleAction(MAX_HEAD_ANGLE);
      }
      
      CompoundActionSequential* action = new CompoundActionSequential({
        new CompoundActionParallel({
          new WaitAction(_waitAfterButton_sec),
          moveHeadAction
        }),
        
        // One...
        new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK), 
        new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK+10), 
        new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK), 
        new WaitAction(_waitTimeBetweenTaps_sec),

        // Two...
        new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK), 
        new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK+10), 
        new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK), 
        new WaitAction(_waitTimeBetweenTaps_sec),

        // Three...
        new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK), 
        new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK+10), 
        new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK),
        new WaitAction(_waitTimeBetweenTaps_sec),

        // Shoot!
        new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK), 
        new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK+10),
        new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK),
      });

      DelegateIfInControl(action, [&bei,this]() {
        DisplaySelection(bei);
        auto& visionComponent = bei.GetVisionComponent();
        visionComponent.EnableMode(VisionMode::DetectingGeneralObjects, true);
      });
      
      _state = State::WaitForResult;
      
      break;
    }

    case State::WaitForResult:
    {
      // Just waiting... Handler for ObjectDetected message will change state for us
      break;
    }

    case State::React:
    {
      if(!IsActing())
      {
        auto& visionComponent = bei.GetVisionComponent();
        visionComponent.EnableMode(VisionMode::DetectingGeneralObjects, false);
        
        static const AnimationTrigger kWinTrigger     = AnimationTrigger::CubePounceWinSession;
        static const AnimationTrigger kLoseTrigger    = AnimationTrigger::CubePounceLoseSession;
        static const AnimationTrigger kDrawTrigger    = AnimationTrigger::CubePouncePounceNormal;
        static const AnimationTrigger kUnknownTrigger = AnimationTrigger::RequestGameInterrupt;
        
        // Results:
        //   Robot's selection is first, Human's is second
        static const std::map<std::pair<Selection,Selection>, AnimationTrigger> animationLUT{
          // Win:
          {{Selection::Rock,     Selection::Scissors},     kWinTrigger},
          {{Selection::Paper,    Selection::Rock},         kWinTrigger},
          {{Selection::Scissors, Selection::Paper},        kWinTrigger},
          
          // Lose:
          {{Selection::Rock,     Selection::Paper},        kLoseTrigger},
          {{Selection::Paper,    Selection::Scissors},     kLoseTrigger},
          {{Selection::Scissors, Selection::Rock},         kLoseTrigger},
          
          // Draw:
          {{Selection::Rock,     Selection::Rock},         kDrawTrigger},
          {{Selection::Paper,    Selection::Paper},        kDrawTrigger},
          {{Selection::Scissors, Selection::Scissors},     kDrawTrigger},
          
          // Unknown:
          {{Selection::Unknown,  Selection::Scissors},     kUnknownTrigger},
          {{Selection::Unknown,  Selection::Rock},         kUnknownTrigger},
          {{Selection::Unknown,  Selection::Paper},        kUnknownTrigger},
          {{Selection::Unknown,  Selection::Unknown},      kUnknownTrigger},
          {{Selection::Rock,     Selection::Unknown},      kUnknownTrigger},
          {{Selection::Paper,    Selection::Unknown},      kUnknownTrigger},
          {{Selection::Scissors, Selection::Unknown},      kUnknownTrigger},
        };
        
        static const std::map<AnimationTrigger, const ColorRGBA&> lightLUT{
          {kWinTrigger,     NamedColors::GREEN},
          {kLoseTrigger,    NamedColors::RED},
          {kDrawTrigger,    NamedColors::CYAN},
          {kUnknownTrigger, NamedColors::YELLOW},
        };
        
        const AnimationTrigger animToPlay = animationLUT.at(std::pair<Selection,Selection>(_robotSelection,_humanSelection));
        
        const ColorRGBA& lightColor = lightLUT.at(animToPlay);
        const BackpackLights lights{
          .onColors               = {{lightColor,lightColor,lightColor}},
          .offColors              = {{lightColor,lightColor,lightColor}},
          .onPeriod_ms            = {{0,0,0}},
          .offPeriod_ms           = {{0,0,0}},
          .transitionOnPeriod_ms  = {{0,0,0}},
          .transitionOffPeriod_ms = {{0,0,0}},
          .offset                 = {{0,0,0}}
        };
        
        bei.GetBodyLightComponent().SetBackpackLights(lights);
        
        DelegateIfInControl(new TriggerAnimationAction(animToPlay), [this,&bei](){
          _robotSelection = Selection::Unknown;
          _humanSelection = Selection::Unknown;
          bei.GetBodyLightComponent().SetBackpackLights( kLightsOff );
          _state = WaitForButton;
        });
      }
      break;
    }
  }
  
  // always stay running
  return BehaviorStatus::Running;
}

void BehaviorRockPaperScissors::DisplaySelection(BehaviorExternalInterface& bei)
{
  DEV_ASSERT(bei.HasAnimationComponent(),
             "BehaviorRockPaperScissors.DisplaySelection.NoAnimComponent");
  
  _robotSelection = static_cast<Selection>(GetRNG().RandIntInRange(0, 2));
  
  const CozmoContext* context = bei.GetRobotInfo().GetContext();
  const std::string imgPath = context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                         "config/engine/behaviorComponent/behaviors/freeplay/userInteractive/rockPaperScissors");
  std::string filename = "";
  
  switch(_robotSelection)
  {
    case Selection::Rock:     filename = "rock.png";     break;
      
    case Selection::Paper:    filename = "paper.png";    break;
      
    case Selection::Scissors: filename = "scissors.png"; break;
      
    case Selection::Unknown:
      DEV_ASSERT(false, "BehaviorRockPaperScissors.DisplaySelection.UnknownNotPossible");
      return;
  }
  
  Vision::ImageRGB img;
  const std::string fullfilename = Util::FileUtils::FullFilePath({imgPath, filename});
  Result result = img.Load(fullfilename);
  DEV_ASSERT_MSG(RESULT_OK == result, "BehaviorRockPaperScissors.DisplaySelection.NoImage",
                 "%s", fullfilename.c_str());
  
  auto & animComponent = bei.GetAnimationComponent();
  animComponent.DisplayFaceImage(img, _displayHoldTime_ms, true);
}
  
//void BehaviorRockPaperScissors::BlinkLight(BehaviorExternalInterface& bei)
//{
//  _blinkOn = !_blinkOn;
//
//  bei.GetBodyLightComponent().SetBackpackLights( _blinkOn ? kLightsOn : kLightsOff );
//
//  // always blink if we are streaming, and set up another blink for single photo if we need to turn off the
//  // light
//  if( _blinkOn || _isStreaming ) {
//    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
//    // need to do another blink after this one
//    _timeToBlink = currTime_s + kLightBlinkPeriod_s;
//  }
//  else {
//    _timeToBlink = -1.0f;
//  }
//}

void BehaviorRockPaperScissors::HandleWhileActivated(const EngineToGameEvent& event,  BehaviorExternalInterface& behaviorExternalInterface) 
{
  switch(event.GetData().GetTag())
  {
    case ExternalInterface::MessageEngineToGameTag::RobotObservedGenericObject:
    {
      if(State::WaitForResult == _state)
      {
        _humanSelection = Selection::Unknown;
        
        auto const& detection = event.GetData().Get_RobotObservedGenericObject();
        if(detection.score > _minDetectionScore)
        {
          if(detection.name == "rock")
          {
            _humanSelection = Selection::Rock;
          }
          else if(detection.name == "paper")
          {
            _humanSelection = Selection::Paper;
          }
          else if(detection.name == "scissors")
          {
            _humanSelection = Selection::Scissors;
          }
          
          PRINT_NAMED_INFO("BehaviorRockPaperScissors.HandleWhileActivated.HumanSelectionDetected",
                           "%s:%f", detection.name.c_str(), detection.score);
        }
        
        _state = State::React;
      }
      break;
    }
      
    default:
    {
      PRINT_NAMED_WARNING("BehaviorRockPaperScissors.HandleWhileActivated.UnexpectedEngineToGameEvent",
                          "%s", MessageEngineToGameTagToString(event.GetData().GetTag()));
    }
  }
}

}
}

