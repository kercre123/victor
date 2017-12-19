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

#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"

namespace Anki {
namespace Cozmo {

namespace {

CONSOLE_VAR(bool, kRockPaperScissors_FakePowerButton, "RockPaperScissors", false);
//  
//constexpr const float kLightBlinkPeriod_s = 0.5f;
//constexpr const float kHoldTimeForStreaming_s = 1.0f;
//
//static const BackpackLights kLightsOn = {
//  .onColors               = {{NamedColors::BLACK,NamedColors::GREEN,NamedColors::BLACK}},
//  .offColors              = {{NamedColors::BLACK,NamedColors::GREEN,NamedColors::BLACK}},
//  .onPeriod_ms            = {{0,0,0}},
//  .offPeriod_ms           = {{0,0,0}},
//  .transitionOnPeriod_ms  = {{0,0,0}},
//  .transitionOffPeriod_ms = {{0,0,0}},
//  .offset                 = {{0,0,0}}
//};
//
//static const BackpackLights kLightsOff = {
//  .onColors               = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
//  .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
//  .onPeriod_ms            = {{0,0,0}},
//  .offPeriod_ms           = {{0,0,0}},
//  .transitionOnPeriod_ms  = {{0,0,0}},
//  .transitionOffPeriod_ms = {{0,0,0}},
//  .offset                 = {{0,0,0}}
//};

}

BehaviorRockPaperScissors::BehaviorRockPaperScissors(const Json::Value& config)
  : ICozmoBehavior(config)
{
  SubscribeToTag(ExternalInterface::MessageEngineToGameTag::RobotObservedGenericObject);
  
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
      if(bei.GetRobotInfo().IsPowerButtonPressed() || kRockPaperScissors_FakePowerButton)
      {
        kRockPaperScissors_FakePowerButton = false;
        _state = State::PlayCadence;
      }
      break;
    }

    case State::PlayCadence:
    {
      // TODO: Make configurable
      const float _waitAfterButton_sec = 1.f;
      const float _waitTimeBetweenTaps_sec = 0.5f;

      CompoundActionSequential* action = new CompoundActionSequential({
        new WaitAction(_waitAfterButton_sec),
        
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
        new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK+20), 
        new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK),
      });

      DelegateIfInControl(action, [&bei,this]() {
        DisplaySelection(bei);
        auto& visionComponent = bei.GetVisionComponent();
        visionComponent.EnableMode(VisionMode::DetectingGeneralObjects, true);
        _state = State::WaitForResult;
      }); 
      break;
    }

    case State::WaitForResult:
    {
      // Just waiting... Handler for ObjectDetected message will change state for us
      break;
    }

    case State::React:
    {
      auto& visionComponent = bei.GetVisionComponent();
      visionComponent.EnableMode(VisionMode::DetectingGeneralObjects, false);
      
      enum GameResult {
        Win,
        Lose,
        Draw,
        Unknown
      } gameResult = Unknown;
      
      static const std::unordered_map<GameResult, AnimationTrigger> animationLUT{
        {GameResult::Win,     AnimationTrigger::CubePounceWinSession},
        {GameResult::Lose,    AnimationTrigger::CubePounceLoseSession},
        {GameResult::Draw,    AnimationTrigger::CubePouncePounceNormal},
        {GameResult::Unknown, AnimationTrigger::RequestGameInterrupt},
      };
      
      switch(_humanSelection)
      {
        case Selection::Unknown:   gameResult = Unknown;  break;
          
        case Selection::Rock:
        {
          switch(_robotSelection)
          {
            case Selection::Rock:     gameResult = Draw;     break;
            case Selection::Paper:    gameResult = Win;      break;
            case Selection::Scissors: gameResult = Lose;     break;
            case Selection::Unknown:
              DEV_ASSERT(false, "BehaviorRockPaperScissors.UpdateInternal_WhileRunning.UnknownNotPossible");
              gameResult = Unknown;
              break;
          }
          break;
        }
          
        case Selection::Paper:
        {
          switch(_robotSelection)
          {
            case Selection::Rock:     gameResult = Lose;     break;
            case Selection::Paper:    gameResult = Draw;     break;
            case Selection::Scissors: gameResult = Win;      break;
            case Selection::Unknown:
              DEV_ASSERT(false, "BehaviorRockPaperScissors.UpdateInternal_WhileRunning.UnknownNotPossible");
              gameResult = Unknown;
              break;
          }
          break;
        }
          
        case Selection::Scissors:
        {
          switch(_robotSelection)
          {
            case Selection::Rock:     gameResult = Win;      break;
            case Selection::Paper:    gameResult = Lose;     break;
            case Selection::Scissors: gameResult = Draw;     break;
            case Selection::Unknown:
              DEV_ASSERT(false, "BehaviorRockPaperScissors.UpdateInternal_WhileRunning.UnknownNotPossible");
              gameResult = Unknown;
              break;
          }
          break;
        }
      }
      
      DelegateIfInControl(new TriggerAnimationAction(animationLUT.at(gameResult)), [this](){
        _state = WaitForButton;
      });

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
  const std::string imgPath = context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources, "assets/rockPaperScissors");
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
  animComponent.DisplayFaceImage(img, 0, true);
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
        // TODO: Make configurable
        const float _minScore = 0.7f;
        
        _humanSelection = Selection::Unknown;
        
        auto const& detection = event.GetData().Get_RobotObservedGenericObject();
        if(detection.score > _minScore)
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

