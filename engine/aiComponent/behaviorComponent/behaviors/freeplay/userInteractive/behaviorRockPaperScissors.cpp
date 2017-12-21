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
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/components/bodyLightComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"

#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/quoteMacro.h"

namespace Anki {
namespace Cozmo {

// Transition to a new state and update the debug name for logging/debugging
#define SET_STATE(s) do{ _state = State::s; DEBUG_SET_STATE(s); } while(0)
  
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
  GET_FROM_CONFIG(Uint32, tapHeight_mm);
  GET_FROM_CONFIG(Bool,   showDetectionString);
  GET_FROM_CONFIG(Uint8, shootOnCount);
  
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

  SET_STATE(WaitForButton);
  _humanSelection = Selection::Unknown;
  _robotSelection = Selection::Unknown;
  
  // Start with detection disabled to save heat/power
  auto& visionComponent = bei.GetVisionComponent();
  visionComponent.EnableMode(VisionMode::DetectingGeneralObjects, false);
  
  //bei.GetRobotAudioClient().SetRobotMasterVolume(0.5f);
  
  if(_displayImages.empty())
  {
    const CozmoContext* context = bei.GetRobotInfo().GetContext();
    const std::string imgPath = context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                           "config/engine/behaviorComponent/behaviors/freeplay/userInteractive/rockPaperScissors");
    
    Result result = _displayImages[Selection::Rock].Load(Util::FileUtils::FullFilePath({imgPath, "rock.png"}));
    DEV_ASSERT_MSG(RESULT_OK == result, "BehaviorRockPaperScissors.DisplaySelection.NoRockImage",
                   "%s", imgPath.c_str());
    
    result = _displayImages[Selection::Paper].Load(Util::FileUtils::FullFilePath({imgPath, "paper.png"}));
    DEV_ASSERT_MSG(RESULT_OK == result, "BehaviorRockPaperScissors.DisplaySelection.NoPaperImage",
                   "%s", imgPath.c_str());
    
    result = _displayImages[Selection::Scissors].Load(Util::FileUtils::FullFilePath({imgPath, "scissors.png"}));
    DEV_ASSERT_MSG(RESULT_OK == result, "BehaviorRockPaperScissors.DisplaySelection.NoScissorsImage",
                   "%s", imgPath.c_str());
    
    // TODO: Add question mark image?
    _displayImages[Selection::Unknown].Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH/2);
    _displayImages[Selection::Unknown].FillWith(0);
    
  }
  
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
        SET_STATE(PlayCadence);
      }
      break;
    }

    case State::PlayCadence:
    {
      if(!IsActing())
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
        
        CompoundActionSequential* compoundAction = new CompoundActionSequential({
          new CompoundActionParallel({
            new WaitAction(_waitAfterButton_sec),
            moveHeadAction
          }),
          new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK),
        });
        
        for(s32 iUpDown=0; iUpDown < _shootOnCount; ++iUpDown)
        {
          //const float moveDuration_sec = 0.1f;
          const float tolerance_mm = 15.f; // just won't movement, not accuracy that slows us down
          MoveLiftToHeightAction* upAction = new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK+_tapHeight_mm, tolerance_mm);
          //upAction->SetDuration(moveDuration_sec);
          upAction->SetWaitUntilMovementStops(false);
          
          MoveLiftToHeightAction* downAction = new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK, tolerance_mm);
          //downAction->SetDuration(moveDuration_sec);
          downAction->SetWaitUntilMovementStops(false);
          
          compoundAction->AddAction(upAction);
          compoundAction->AddAction(downAction);
          compoundAction->AddAction(new WaitAction(_waitTimeBetweenTaps_sec));
        }
        
        DelegateIfInControl(compoundAction, [&bei,this]()
                            {
                              // Start looking for the hand and display our selection
                              auto& visionComponent = bei.GetVisionComponent();
                              visionComponent.EnableMode(VisionMode::DetectingGeneralObjects, true);
                              
                              _robotSelection = static_cast<Selection>(GetRNG().RandIntInRange(0, 2));
                              
                              SET_STATE(WaitForResult);
                              
                              //        DelegateIfInControl(new CompoundActionParallel({
                              //          new WaitForLambdaAction([this,&bei](Robot&){
                              //            DisplaySelection(bei);
                              //            return true;
                              //          }),
                              //          new WaitAction(Util::MilliSecToSec((float)_displayHoldTime_ms))
                              //        }));
                            });
      }
      
      break;
    }

    case State::WaitForResult:
    {
      // Just waiting... Handler for ObjectDetected message will change state for us
      DisplaySelection(bei);
      break;
    }
      
    case State::ShowResult:
    {
      if(_humanSelection != Selection::Unknown)
      {
        if(IsActing())
        {
          DisplaySelection(bei);
        }
        else
        {
          DelegateIfInControl(new WaitAction(Util::MilliSecToSec((float)_displayHoldTime_ms)), [this]()
                              {
                                SET_STATE(React);
                              });
        }
      }
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
        
        bei.GetRobotInfo().GetMoveComponent().LockTracks(static_cast<u8>(AnimTrackFlag::BACKPACK_LIGHTS_TRACK),
                                                         "RockPaperScissors", "RockPaperScissors");
        
        bei.GetBodyLightComponent().SetBackpackLights(lights);
        
        DelegateIfInControl(new TriggerAnimationAction(animToPlay), [this,&bei](){
          _robotSelection = Selection::Unknown;
          _humanSelection = Selection::Unknown;
          bei.GetBodyLightComponent().SetBackpackLights( kLightsOff );
          bei.GetRobotInfo().GetMoveComponent().UnlockTracks(static_cast<u8>(AnimTrackFlag::BACKPACK_LIGHTS_TRACK), "RockPaperScissors");
          SET_STATE(WaitForButton);
        });
      }
      break;
    }
  }
  
  // always stay running
  return BehaviorStatus::Running;
}

void BehaviorRockPaperScissors::CopyToHelper(Vision::ImageRGB& toImg, const Selection selection, const s32 xOffset, const bool swapGB) const
{
  const Vision::ImageRGB& selectedImg = _displayImages.at(selection);
  DEV_ASSERT(selectedImg.GetNumRows() <= toImg.GetNumRows(), "BehaviorRockPaperScissors.DisplaySelection.BadNumRows");
  DEV_ASSERT(selectedImg.GetNumCols() + xOffset <= toImg.GetNumCols(), "BehaviorRockPaperScissors.DisplaySelection.BadNumRows");
  for(s32 i=0; i<selectedImg.GetNumRows(); ++i)
  {
    const Vision::PixelRGB* selectedImg_i = selectedImg.GetRow(i);
    Vision::PixelRGB* img_i = toImg.GetRow(i) + xOffset;
    
    for(s32 j=0; j<selectedImg.GetNumCols(); ++j)
    {
      const Vision::PixelRGB& selectedPixel = selectedImg_i[j];
      Vision::PixelRGB& imgPixel = img_i[j];
      
      imgPixel = selectedPixel;
      if(swapGB)
      {
        std::swap(imgPixel.g(), imgPixel.b());
      }
    }
  }
}
  
void BehaviorRockPaperScissors::DisplaySelection(BehaviorExternalInterface& bei, const std::string& overlayStr)
{
  DEV_ASSERT(bei.HasAnimationComponent(),
             "BehaviorRockPaperScissors.DisplaySelection.NoAnimComponent");
  
  std::string filename = "";
  
  static Vision::ImageRGB img(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  
  if(_humanSelection == Selection::Unknown)
  {
    img.FillWith(0);
//    Rectangle<s32> rect(FACE_DISPLAY_WIDTH/4, 0, FACE_DISPLAY_WIDTH/2, FACE_DISPLAY_HEIGHT);
//    Vision::ImageRGB robotROI = img.GetROI(rect);
//    _displayImages[_robotSelection].CopyTo(robotROI);
//
    CopyToHelper(img, _robotSelection, FACE_DISPLAY_WIDTH/4, false);
  }
  else
  {
    {
      Rectangle<s32> robotRect(0, 0, FACE_DISPLAY_WIDTH/2, FACE_DISPLAY_HEIGHT);
      Vision::ImageRGB robotROI = img.GetROI(robotRect);
      _displayImages.at(_robotSelection).CopyTo(robotROI);
    }

    // WTF? Why does this ROI not work when the one above does?
    // Had to manually copy in the loop below... no idea why.
    //    {
    //      Rectangle<s32> humanRect(FACE_DISPLAY_WIDTH/2, 0, FACE_DISPLAY_WIDTH/2, FACE_DISPLAY_HEIGHT);
    //      Vision::ImageRGB humanROI = img.GetROI(humanRect);
    //      _displayImages.at(_humanSelection).CopyTo(humanROI);
    //    }
    //    // Cheap hack to change human display color
    //    for(s32 i=0; i<humanROI.GetNumRows(); ++i)
    //    {
    //      Vision::PixelRGB* humanROI_i = humanROI.GetRow(i);
    //      for(s32 j=0; j<humanROI.GetNumCols(); ++j)
    //      {
    //        std::swap(humanROI_i[j].g(), humanROI_i[j].b());
    //      }
    //    }
    CopyToHelper(img, _robotSelection, 0, false);
    CopyToHelper(img, _humanSelection, FACE_DISPLAY_WIDTH/2, true);
  }
  
  auto & animComponent = bei.GetAnimationComponent();
  animComponent.DisplayFaceImage(img, BS_TIME_STEP, true);
}
  
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
          
//          if(_showDetectionString)
//          {
//            std::string overlayStr(detection.name);
//            overlayStr += ":";
//            overlayStr += std::to_string((s32)std::round(100.f*detection.score));
//            DisplaySelection(behaviorExternalInterface, overlayStr);
//          }
          
          if(_humanSelection != Selection::Unknown)
          {
            PRINT_NAMED_INFO("BehaviorRockPaperScissors.HandleWhileActivated.HumanSelectionDetected",
                             "%s:%f", detection.name.c_str(), detection.score);
            SET_STATE(ShowResult);
          }
        }
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

