/**
 * File: BehaviorBoxDemoDescribeScene.cpp
 *
 * Author: Andrew Stein
 * Created: 2019-01-09
 *
 * Description: Shows text describing the scene when asked or when The Box is repositioned
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/vision/engine/image_impl.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/sayTextAction.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/box/behaviorBoxDemoDescribeScene.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/salientPointsComponent.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/components/visionComponent.h"

namespace Anki {
namespace Vector {

namespace ConfigKeys
{
  const char* const kDelocTimeWindow = "delocTimeWindow_sec";
  const char* const kTextDisplayTime = "textDisplayTime_sec";
  const char* const kVisionTimeout   = "visionRequestTimeout_sec";
}
 
// Not in anonymous namespace so they are visible to demo web page
CONSOLE_VAR(bool, kTheBox_TTSForDescription, "TheBox.Audio", true);
CONSOLE_VAR(bool, kTheBox_TTSForOCR,         "TheBox.Audio", false);
  
namespace
{
  const UserIntentTag kUserIntent_DescribeScene = USER_INTENT(describe_scene);
  const UserIntentTag kUserIntent_OCR = USER_INTENT(perform_ocr);
  
  // Enable to have this behavior trigger when TheBox is moved (after actived once with voice)
  // If false, behavior only triggers on voice command
  CONSOLE_VAR(bool, kTheBox_TriggerDescribeSceneOnMove, "TheBox", false);
  
  CONSOLE_VAR_RANGED(f32, kTheBox_DescribeSceneTextScale, "TheBox.Screen", 0.65f, 0.1, 1.f);
  CONSOLE_VAR_RANGED(s32, kTheBox_DescribeSceneLineSpacing_pix, "TheBox.Screen", 2, 1, 10);
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoDescribeScene::InstanceConfig::InstanceConfig()
{
  Json::Value config;
  config["conditionType"] = "RobotTouched";
  config["minTouchTime"] = 0.2f;
  config["waitForRelease"] = true;
  config["waitForReleaseTimeout_s"] = 1.0f;

  touchAndReleaseCondition = BEIConditionFactory::CreateBEICondition(config,
                                                                     "BehaviorBoxDemoDescribeScene.TouchAndRelease");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoDescribeScene::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoDescribeScene::BehaviorBoxDemoDescribeScene(const Json::Value& config)
 : ICozmoBehavior(config)
{
  _iConfig.recentDelocTimeWindow_sec = JsonTools::ParseFloat(config, ConfigKeys::kDelocTimeWindow,
                                                             "BehaviorBoxDemoDescribeScene");
  
  _iConfig.textDisplayTime_sec = JsonTools::ParseFloat(config, ConfigKeys::kTextDisplayTime,
                                                       "BehaviorBoxDemoDescribeScene");
  
  _iConfig.visionRequestTimeout_sec = JsonTools::ParseFloat(config, ConfigKeys::kVisionTimeout,
                                                            "BehaviorBoxDemoDescribeScene");
  
  _iConfig.blockWorldFilter = std::make_unique<BlockWorldFilter>();
  _iConfig.blockWorldFilter->AddAllowedType(ObjectType::CustomType00);
  
  _iConfig.blockWorldFilter->AddFilterFcn([this](const ObservableObject* object)
  {
    // Kinda gross to get these for _each_ object, but better than creating the whole filter every tick?
    const TimeStamp_t lastImageTime_ms = (TimeStamp_t)GetBEI().GetVisionComponent().GetLastProcessedImageTimeStamp();
    
    if(lastImageTime_ms - object->GetLastObservedTime() < _iConfig.recentDelocTimeWindow_sec)
    {
      return true;
    }
    return false;
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoDescribeScene::~BehaviorBoxDemoDescribeScene()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBoxDemoDescribeScene::WantsToBeActivatedBehavior() const
{
  if(kTheBox_TriggerDescribeSceneOnMove)
  {
    const bool hasBehaviorEverRun = ( GetNumTimesBehaviorStarted() > 0 );
    auto& aiwb = GetAIComp<AIWhiteboard>();
    const float secSinceLastDeloc = aiwb.GetSecondsSinceLastDelocalization();
    const bool delocalizedRecently = (secSinceLastDeloc < _iConfig.recentDelocTimeWindow_sec);
    const bool isOnTreads = (GetBEI().GetRobotInfo().GetOffTreadsState() == OffTreadsState::OnTreads);
    if(hasBehaviorEverRun && delocalizedRecently && isOnTreads)
    {
      // when the box is put down, execute this behavior, but only after it's been started at least once
      // manually or with the command
      return true;
    }
  }
  
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  const bool pendingIntent = (uic.IsUserIntentPending(kUserIntent_DescribeScene) ||
                              uic.IsUserIntentPending(kUserIntent_OCR));
  if(pendingIntent)
  {
    return true;
  }
  
  const BlockWorld& blockWorld = GetBEI().GetBlockWorld();
  const ObservableObject* obj = blockWorld.FindLocatedMatchingObject(*_iConfig.blockWorldFilter);
  if(obj != nullptr)
  {
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoDescribeScene::InitBehavior()
{
  _iConfig.touchAndReleaseCondition->Init(GetBEI());
  //_iConfig.showTextBehavior = FindBehaviorByID(BehaviorID::ShowText);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoDescribeScene::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  
  modifiers.visionModesForActivatableScope->insert({VisionMode::DetectingMarkers, EVisionUpdateFrequency::Low});
  modifiers.visionModesForActivatableScope->insert({VisionMode::FullHeightMarkerDetection, EVisionUpdateFrequency::High});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoDescribeScene::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  // TODO: insert any behaviors this will delegate to into delegates.
  // TODO: delete this function if you don't need it
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoDescribeScene::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    ConfigKeys::kDelocTimeWindow,
    ConfigKeys::kTextDisplayTime,
    ConfigKeys::kVisionTimeout,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoDescribeScene::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  auto& uic = GetBehaviorComp<UserIntentComponent>();
  if(uic.IsUserIntentPending(kUserIntent_DescribeScene))
  {
    SmartActivateUserIntent(kUserIntent_DescribeScene);
  }
  else if(uic.IsUserIntentPending(kUserIntent_OCR))
  {
    SmartActivateUserIntent(kUserIntent_OCR);
    _dVars.isUsingOCR = true;
  }
  else
  {
    const BlockWorld& blockWorld = GetBEI().GetBlockWorld();
    const ObservableObject* obj = blockWorld.FindLocatedMatchingObject(*_iConfig.blockWorldFilter);
    if(obj != nullptr)
    {
      _dVars.isUsingOCR = true;
    }
  }
  
  _iConfig.touchAndReleaseCondition->SetActive(GetBEI(), true);
  
  CompoundActionParallel* processingAction = new CompoundActionParallel();

  {
    PlayAnimationAction* playAnim = new PlayAnimationAction("anim_elemental_processing", 0);
    playAnim->SetRenderInEyeHue(false);

    processingAction->AddAction(playAnim);
  }
  
  _dVars.lastImageTime_ms = GetBEI().GetVisionComponent().GetLastProcessedImageTimeStamp();

  {
    const VisionMode visionMode = (_dVars.isUsingOCR ? VisionMode::OffboardOCR : VisionMode::OffboardSceneDescription);
    WaitForImagesAction* waitAction = new WaitForImagesAction(1, visionMode, _dVars.lastImageTime_ms);
    waitAction->SetTimeoutInSeconds(_iConfig.visionRequestTimeout_sec); // in case we never hear back from vision

    processingAction->AddAction(waitAction);
  }

  processingAction->SetShouldEndWhenFirstActionCompletes(true);
  
  DelegateIfInControl(processingAction, [this](ActionResult result)
                      {
                        if(result == ActionResult::SUCCESS)
                        {
                          DisplayDescription();
                        }
                        else
                        {
                          using GE = AudioMetaData::GameEvent::GenericEvent;
                          using GO = AudioMetaData::GameObjectType;
                          GetBEI().GetRobotAudioClient().PostEvent(GE::Play__Robot_Vic_Sfx__Emote_Cant_Do_That_1,
                                                                   GO::Behavior);
                        }
                      });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoDescribeScene::OnBehaviorDeactivated()
{
  _iConfig.touchAndReleaseCondition->SetActive(GetBEI(), false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void BreakIntoLines(const std::string& description, const float imageWidth,
                           std::vector<std::string>& lines, f32& lineHeight)
{
  // Traverse the description
  std::istringstream descriptionStream(description);
  std::string nextLine;
  float nextLineLength = 0.f;
  lineHeight = 0.f;
  const float spaceLength = Vision::ImageRGB::GetTextSize(" ", kTheBox_DescribeSceneTextScale, 1).x();
  do {
    // Grab next word from the description
    std::string word;
    descriptionStream >> word;
    
    const Vec2f wordSize = Vision::ImageRGB::GetTextSize(word, kTheBox_DescribeSceneTextScale, 1);
    if(!word.empty() && (nextLineLength + wordSize.x() < imageWidth))
    {
      nextLine += word + " ";
      nextLineLength += wordSize.x() + spaceLength;
      lineHeight = std::max(lineHeight, wordSize.y());
    }
    else
    {
      lines.emplace_back(nextLine);
      nextLine = word + " ";
      nextLineLength = wordSize.x() + spaceLength;
    }
  } while(descriptionStream);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoDescribeScene::DisplayDescription()
{
  // TODO: is there a pretty way to reuse (delegate to) CheckForAndReactToSalientPoint here instead?
  const SalientPointsComponent& salientPointsComp = GetAIComp<SalientPointsComponent>();
  
  const Vision::SalientPointType salientType = (_dVars.isUsingOCR ?
                                                Vision::SalientPointType::Text :
                                                Vision::SalientPointType::SceneDescription);
  
  std::list<Vision::SalientPoint> salientPoints;
  salientPointsComp.GetSalientPointSinceTime(salientPoints, salientType, _dVars.lastImageTime_ms);
  if(!salientPoints.empty())
  {
    auto iter = salientPoints.begin();
    const std::string* newestDescription = &(iter->description);
    if(salientPoints.size() > 1)
    {
      LOG_WARNING("BehaviorBoxDemoDescribeScene.DisplayDescription.MultipleSalientPoints",
                  "Got %zu instead of expected 1. Finding newest.", salientPoints.size());
      
      TimeStamp_t newestTime = iter->timestamp;
      ++iter;
      while (iter != salientPoints.end())
      {
        if(iter->timestamp > newestTime)
        {
          newestTime = iter->timestamp;
          newestDescription = &(iter->description);
        }
        ++iter;
      }
    }
    
    Vision::ImageRGB dispImg(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
    dispImg.FillWith(Vision::PixelRGB(0,0,0));
    
    std::vector<std::string> lines;
    float lineHeight = 0.f;
    BreakIntoLines(*newestDescription, dispImg.GetNumCols(), lines, lineHeight);
    
    // Draw all the lines of text
    s32 lineNum = 1;
    for(const auto& line : lines)
    {
      const bool kDropShadow = true;
      const s32  kThickness = 1;
      const f32  ypos = lineNum*(lineHeight+kTheBox_DescribeSceneLineSpacing_pix);
      if(ypos >= dispImg.GetNumRows())
      {
        break;
      }
      dispImg.DrawText({1.f, ypos}, line, NamedColors::YELLOW,
                       kTheBox_DescribeSceneTextScale, kDropShadow, kThickness);
      ++lineNum;
    }
    
    const bool kInterruptRunning = true;
    GetBEI().GetAnimationComponent().DisplayFaceImage(dispImg,
                                                      AnimationComponent::DEFAULT_STREAMING_FACE_DURATION_MS,
                                                      kInterruptRunning);

    const float startTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

    CompoundActionParallel* responseAction = new CompoundActionParallel();
    responseAction->AddAction( new WaitForLambdaAction([this, startTime_s](Robot& r) {
          if( _iConfig.touchAndReleaseCondition->AreConditionsMet(GetBEI()) ) {
            // touch and release done, time to stop the text
            return true;
          }

          const bool pressed = GetBEI().GetTouchSensorComponent().GetIsPressed();
          if( pressed ) {
            // while pressed, keep displaying the text
            return false;
          }

          // otherwise, display for the given timeout
          const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

          const bool timedOut = (currTime_s - startTime_s) > _iConfig.textDisplayTime_sec;
          return timedOut;
        }));

    if( (_dVars.isUsingOCR && kTheBox_TTSForOCR) || (!_dVars.isUsingOCR && kTheBox_TTSForDescription) )
    {
      responseAction->AddAction( new SayTextAction(salientPoints.begin()->description + ".",
                                                   SayTextAction::AudioTtsProcessingStyle::Unprocessed));
    }
    
    DelegateIfInControl(responseAction);
  }
}

}
}
